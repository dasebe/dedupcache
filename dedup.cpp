#include <fstream>
#include <string>
#include <regex>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include "cache/lru_variants.h"
#include "cache/request.h"

using namespace std;

int main (int argc, char* argv[])
{
    const uint64_t metaCap  = std::stoull(argv[1]);
    const uint64_t dataCap  = std::stoull(argv[2]);

    // trace properties
    std::cerr << "startup\n";
    int64_t reqs = 0, reusematch = 0, storagematch = 0;
    int64_t ts, pid, blocksize, majordev, minordev;
    std::string proc, lba, rwflag, hash;

    // cache init
    std::cerr << "cache init\n";
    const string cacheType = "LRU";
    unique_ptr<Cache> metaCache = move(Cache::create_unique(cacheType));
    unique_ptr<Cache> blockCache = move(Cache::create_unique(cacheType));
    if(metaCache == nullptr || blockCache == nullptr)
        return 1;
    metaCache->setSize(metaCap);
    blockCache->setSize(dataCap);
    std::cerr << "cache init2\n";
    SimpleRequest* reqLBA = new SimpleRequest("64",1);
    SimpleRequest* reqBlock = new SimpleRequest("64",1);
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
    for(int i=3; i<argc; i++) {
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

            // cache lookup
            reqLBA->reinit(lba,64);
            //            std::cerr << "lookup1\n";
            metaHit = metaCache->lookup(reqLBA);
            reqBlock->reinit(hash,4096);
            //            std::cerr << "lookup2\n";
            blockHit = blockCache->lookup(reqBlock);
            // cache admission
            if(metaHit && blockHit) {
                fullHit++;
            } else if(!metaHit) {
                falseNeg++;
                metaCache->admit(reqLBA);
            } else if(!blockHit) {
                falsePos++;
                blockCache->admit(reqBlock);
            } else {
                metaCache->admit(reqLBA);
                blockCache->admit(reqBlock);
            }
        }

        infile.close();
        std::cout << i << " " << path << " " << reqs << " " << reusematch << " " << storagematch << " fH " << fullHit << " fN " << falseNeg << " fp " << falsePos << " " << fullHit/double(reqs) << "\n";
        
    }
    return 0;
}
