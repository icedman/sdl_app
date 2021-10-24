#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN64
#include <windows.h>
#include <winioctl.h>
#else
#include <sys/ioctl.h>
#endif

#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "util.h"

#define PRELOAD_LOOP 1
#define MAX_PRELOAD_DEPTH 4

static bool compareFile(std::shared_ptr<struct fileitem_t> f1, std::shared_ptr<struct fileitem_t> f2)
{
    if (f1->isDirectory && !f2->isDirectory) {
        return true;
    }
    if (!f1->isDirectory && f2->isDirectory) {
        return false;
    }
    return f1->name < f2->name;
}

struct fileitem_t* parentItem(struct fileitem_t* item, std::vector<struct fileitem_t*>& list)
{
    int depth = item->depth;
    int i = item->lineNumber;
    while (i-- > 0) {
        struct fileitem_t* res = list[i];
        if (!res) {
            break;
        }
        if (res->depth < depth) {
            return res;
        }
    }
    return item;
}

fileitem_t::fileitem_t()
    : depth(0)
    , expanded(false)
    , isDirectory(false)
    , canLoadMore(false)
{
}

fileitem_t::fileitem_t(std::string p)
    : fileitem_t()
{
    setPath(p);
}

void fileitem_t::setPath(std::string p)
{
    path = p;
    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = split_path(p, delims);
    name = spath.back();

    char* cpath = (char*)malloc(path.length() + 1 * sizeof(char));
    strcpy(cpath, path.c_str());
    expand_path((char**)(&cpath));
    fullPath = std::string(cpath);
    free(cpath);

    // log("--------------------");
    // log("name: %s", name.c_str());
    // log("path: %s", path.c_str());
    // log("fullPath: %s", fullPath.c_str());
}

static struct explorer_t* explorerInstance = 0;

struct explorer_t* explorer_t::instance()
{
    return explorerInstance;
}

explorer_t::explorer_t()
    : currentItem(-1)
    , regenerateList(true)
{
    loadDepth = 0;
    allFilesLoaded = false;

    explorerInstance = this;
}

void explorer_t::buildFileList(std::vector<struct fileitem_t*>& list, struct fileitem_t* files, int depth, bool deep)
{
    for (auto file : files->files) {
        file->depth = depth;
        file->lineNumber = list.size();
        list.push_back(file.get());
        if (file->expanded || deep) {
            buildFileList(list, file.get(), depth + 1, deep);
        }
    }
}

std::vector<struct fileitem_t*> explorer_t::fileList()
{
    if (!allFiles.size()) {
        buildFileList(allFiles, &files, 0, true);
    }
    return allFiles;
}

void explorer_t::setRootFromFile(std::string path)
{
    fileitem_t file;
    file.setPath(path);
    if (path.length() && path[path.length() - 1] != '/') {
        path = path.erase(path.find(file.name));
    }
    loadFolder(&files, path);
}

void explorer_t::loadFolder(fileitem_t* fileitem, std::string p)
{
    if (p != "") {
        fileitem->setPath(p);
    }

    log("load %s", fileitem->fullPath.c_str());

    int safety = 0;
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(fileitem->fullPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') {
                continue;
            }

            std::string filePath = ent->d_name;
            std::string fullPath = fileitem->path + "/" + filePath;

            size_t pos = fullPath.find("//");
            if (pos != std::string::npos) {
                fullPath.replace(fullPath.begin() + pos, fullPath.begin() + pos + 2, "/");
            }
            std::shared_ptr<struct fileitem_t> file = std::make_shared<struct fileitem_t>(fullPath);

#ifdef WIN64
            file->isDirectory = false;
            DIR* sub = opendir(fullPath.c_str());
            if (sub) {
                file->isDirectory = true;
                closedir(sub);
            }
#else
            file->isDirectory = ent->d_type == DT_DIR;
#endif

            file->canLoadMore = file->isDirectory;

            bool exclude = false;
            if (file->isDirectory) {
                for (auto pat : excludeFolders) {
                    if (filePath == pat) {
                        exclude = true;
                        break;
                    }
                }
            } else {
                std::set<char> delims = { '.' };
                std::vector<std::string> spath = split_path(filePath, delims);
                std::string suffix = "*." + spath.back();
                for (auto pat : excludeFiles) {
                    if (suffix == pat) {
                        exclude = true;
                        break;
                    }
                }
            }

            if (exclude) {
                continue;
            }
            fileitem->files.emplace_back(file);
        }
        closedir(dir);
    }

    sort(fileitem->files.begin(), fileitem->files.end(), compareFile);
}

void explorer_t::preloadFolders()
{
    if (allFilesLoaded) {
        return;
    }

    if (!allFiles.size()) {
        buildFileList(allFiles, &files, 0, true);
    }

    int loaded = 0;
    for (auto item : allFiles) {
        if (item->isDirectory && item->canLoadMore && item->depth == loadDepth) {
            loadFolder(item);

            // printf("load more %d\n", item->depth);

            item->canLoadMore = false;
            if (loaded++ >= PRELOAD_LOOP) {
                break;
            }
        }
    }

    if (loaded == 0) {
        loadDepth++;
    }

    allFilesLoaded = (loaded == 0 && loadDepth >= MAX_PRELOAD_DEPTH);
    if (loaded > 0) {
        regenerateList = true;
    }
}

void explorer_t::update(int delta)
{
    if (!allFilesLoaded) {
        preloadFolders();
    }

    if (regenerateList) {
        regenerateList = false;
        allFiles.clear();
        renderList.clear();
        buildFileList(renderList, &files, 0);
    }
}

void explorer_t::print()
{
    std::vector<struct fileitem_t*> files = fileList();

    for (auto file : files) {
        log(">%s\n", file->fullPath.c_str());
    }
}
