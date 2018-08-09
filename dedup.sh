#!/bin/bash

for size in 268435456 1073741824 4294967296 17179869184 68719476736 274877906944; do
    echo "./dedup 274877906944 ${size} ../fiu/cheetah_mail/cheetah.cs.fiu.edu-110108-113008.{1,2,3,4,5,6,7,8}.blkparse"
done | parallel -j 8 > test.log



