#ifndef HASH_H
#define HASH_H

unsigned int murmur_hash(const void* key, int len, unsigned int seed);
unsigned int hash_combine(int lhs, int rhs);

#endif // HASH_H