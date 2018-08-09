#include <fstream>
#include <string>
#include <regex>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "cache/lru_variants.h"
#include "cache/request.h"

using namespace std;

int main (int argc, char* argv[])
{

    // trace properties
    int64_t reqs = 0, reusematch = 0, storagematch = 0;
    int64_t ts, pid, lba, blocksize, majordev, minordev;
    std::string proc, rwflag, hash;

    // cache init
    const string cacheType = "LRU";
    unique_ptr<Cache> metaCache = move(Cache::create_unique(cacheType));
    unique_ptr<Cache> blockCache = move(Cache::create_unique(cacheType));
    if(metaCache == nullptr || blockCache == nullptr)
        return 1;
    const uint64_t metaCap  = 1073741824L;
    const uint64_t dataCap  = 1073741824L;
    metaCache->setSize(cache_size);
    blockCache->setSize(cache_size);

    // stats
    struct entry {
        std::unordered_set<int64_t> lbas;
        int64_t reqs;
    };
    unordered_map<std::string,entry> matches;

    for(int i=1; i<argc; i++) {
        const char* path = argv[i];
        ifstream infile;
        infile.open(path);

        while (infile >> ts >> pid >> proc >> lba >> blocksize >> rwflag >> majordev >> minordev >> hash)
        {
            reqs++;
            if(matches.count(hash)>0) {
                //                std::cerr << lba << " " << hash << "\n";
                reusematch++;
                if(matches[hash].lbas.count(lba)==0) {
                    storagematch++;
                    matches[hash].lbas.insert(lba);
                }
            } else {
                // first lba
                matches[hash].lbas.insert(lba);
            }
            matches[hash].reqs++;
            //        std::cerr << ts << " " << lba << " " << hash << "\n";
        }

        infile.close();
        std::cout << i << " " << path << " " << reqs << " " << reusematch << " " << storagematch << "\n";
        
    }
    return 0;
}
