#ifndef EXPLORER_H
#define EXPLORER_H

#include <string>
#include <vector>

#include "extension.h"

struct fileitem_t;
typedef std::shared_ptr<struct fileitem_t> fileitem_ptr;

struct fileitem_t {

    fileitem_t();
    fileitem_t(std::string path);

    std::string name;
    std::string path;
    std::string fullPath;
    std::vector<fileitem_ptr> files;

    bool expanded;
    bool isDirectory;
    bool canLoadMore;

    int depth;
    int lineNumber;

    void setPath(std::string path);
};

struct fileitem_t* parentItem(struct fileitem_t* item, std::vector<struct fileitem_t*>& list);

struct explorer_t {

    explorer_t();

    static explorer_t* instance();

    void update(int delta);
    void setRootFromFile(std::string path);

    void preloadFolders();
    void loadFolder(fileitem_t* fileitem, std::string p = "");
    void buildFileList(std::vector<struct fileitem_t*>& list, struct fileitem_t* files, int depth, bool deep = false);

    void print();

    std::vector<struct fileitem_t*> fileList();

    fileitem_t files;
    std::vector<struct fileitem_t*> renderList;
    std::vector<struct fileitem_t*> allFiles;
    bool allFilesLoaded;
    int loadDepth;
    std::string rootPath;

    int currentItem;
    bool regenerateList;

    std::vector<std::string> excludeFiles;
    std::vector<std::string> excludeFolders;
};

#endif // EXPLORER_H
