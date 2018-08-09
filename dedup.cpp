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
    std::cerr << "startup\n";
    int64_t reqs = 0, reusematch = 0, storagematch = 0;
    int64_t ts, pid, blocksize, majordev, minordev;
    std::string proc, lba, rwflag, hash;

    // cache init
    std::cerr << "cache init\n";
    const string cacheType = "LRU";
    const uint64_t metaCap  = 1073741824;
    const uint64_t dataCap  = 1073741824;
    unique_ptr<Cache> metaCache = move(Cache::create_unique(cacheType));
    unique_ptr<Cache> blockCache = move(Cache::create_unique(cacheType));
    if(metaCache == nullptr || blockCache == nullptr)
        return 1;
    metaCache->setSize(metaCap);
    blockCache->setSize(dataCap);
    std::cerr << "cache init2\n";
    SimpleRequest* req = new SimpleRequest("0",1);
    std::cerr << "cache init3\n";
    bool metaHit, blockHit;
    int64_t fullHit = 0, falsePos = 0, falseNeg = 0;

    // stats
    std::cerr << "stats init\n";
    struct entry {
        std::unordered_set<std::string> lbas;
        int64_t reqs;
    };
    unordered_map<std::string,entry> matches;

    std::cerr << "reading data\n";
    for(int i=1; i<argc; i++) {
        const char* path = argv[i];
        ifstream infile;
        infile.open(path);

        while (infile >> ts >> pid >> proc >> lba >> blocksize >> rwflag >> majordev >> minordev >> hash)
        {
            //            std::cerr << ts << " " << lba << " " << hash << "\n";
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
            // cache test
            unordered_map<std::string,bool> test;
            if(test.count(lba)>0) {
                std::cerr << "hit " << lba << "\n";
            } else {
                test[lba] = true;
            }
            // cache lookup
            req->reinit(lba,64);
            //            std::cerr << "lookup1\n";
            metaHit = metaCache->lookup(req);
            req->reinit(hash,4096);
            //            std::cerr << "lookup2\n";
            blockHit = blockCache->lookup(req);
            // cache admission
            if(metaHit && blockHit) {
                fullHit++;
            } else if(!metaHit) {
                falseNeg++;
                metaCache->admit(req);
            } else if(!blockHit) {
                falsePos++;
                blockCache->admit(req);
            } else {
                metaCache->admit(req);
                blockCache->admit(req);
            }
        }

        infile.close();
        std::cout << i << " " << path << " " << reqs << " " << reusematch << " " << storagematch << " fH " << fullHit << " fN " << falseNeg << " fp " << falsePos << "\n";
        
    }
    return 0;
}
