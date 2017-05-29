# fits - fits
python main.py -i ~/win_img/larmor/data/ -o ~/temp/f0 --convert
# fits - stack fits
python main.py -i ~/win_img/larmor/data/ -o ~/temp/fs0 --convert --data-as-stack

# fits - tiff
python main.py -i ~/win_img/larmor/data/ -o ~/temp/t0 --convert --out-format tif
# fits - stack tiff
python main.py -i ~/win_img/larmor/data/ -o ~/temp/ts0 --convert --data-as-stack --out-format tif

# fits - nxs
python main.py -i ~/win_img/larmor/data/ -D ~/win_img/larmor/dark/ -F ~/win_img/larmor/flat/ -o ~/temp/nxs0 --convert --data-as-stack --out-format nxs

# repeat same, but with stack of fits from ~/temp/fs0
# stack fits - fits
python main.py -i ~/temp/fs0 -o ~/temp/f1 --convert
# stack fits - stack fits
python main.py -i ~/temp/fs0 -o ~/temp/fs1 --convert --data-as-stack

# stack fits - tiff
python main.py -i ~/temp/fs0 -o ~/temp/t1 --convert --out-format tif
# stack fits - stack tiff
python main.py -i ~/temp/fs0 -o ~/temp/ts1 --convert --data-as-stack --out-format tif

# stack fits - nxs
python main.py -i ~/temp/fs0 -D ~/win_img/larmor/dark/ -F ~/win_img/larmor/flat/ -o ~/temp/nxs1 --convert --data-as-stack --out-format nxs

# now start tiff to everything else
# tiff - fits
python main.py -i ~/temp/t1 -o ~/temp/f2 --convert --in-format tiff
# tiff - stack fits
python main.py -i ~/temp/t1 -o ~/temp/fs2 --convert --data-as-stack --in-format tiff

# tiff - tiff
python main.py -i ~/temp/t1 -o ~/temp/t2 --convert --out-format tiff --in-format tiff
# tiff - stack tiff
python main.py -i ~/temp/t1 -o ~/temp/ts2 --convert --data-as-stack --out-format tiff --in-format tiff

# need to convert flat and dark also
python main.py -i ~/win_img/larmor/dark/ -o ~/temp/tiff_dark --convert --out-format tiff
python main.py -i ~/win_img/larmor/flat/ -o ~/temp/tiff_flat --convert --out-format tiff
# stack tiff - nxs
python main.py -i ~/temp/t1 -D ~/temp/tiff_dark -F ~/temp/tiff_flat -o ~/temp/nxs2 --convert --data-as-stack --out-format nxs --in-format tiff

# now stack of tiffs to everything else
# stack tiff - fits
python main.py -i ~/temp/ts1 -o ~/temp/f3 --convert --in-format tiff
# stack tiff - stack fits
python main.py -i ~/temp/ts1 -o ~/temp/fs3 --convert --data-as-stack --in-format tiff

# stack tiff - tiff
python main.py -i ~/temp/ts1 -o ~/temp/t3 --convert --out-format tiff --in-format tiff
# stack tiff - stack tiff
python main.py -i ~/temp/ts1 -o ~/temp/ts3 --convert --data-as-stack --out-format tiff --in-format tiff

# stack tiff - nxs
python main.py -i ~/temp/ts1 -D ~/temp/tiff_dark -F ~/temp/tiff_flat -o ~/temp/nxs3 --convert --data-as-stack --out-format nxs --in-format tiff

# now for the last, unpack the nxs to everything else
# nxs - fits
python main.py -i ~/temp/nxs3 -o ~/temp/f4 --convert --in-format nxs
# nxs - stack fits
python main.py -i ~/temp/nxs3 -o ~/temp/f4s --convert --in-format nxs --data-as-stack

# nxs - tiff
python main.py -i ~/temp/nxs3 -o ~/temp/f5 --convert --in-format nxs --out-format tiff
# nxs - stack tiff
python main.py -i ~/temp/nxs3 -o ~/temp/f5s --convert --in-format nxs --data-as-stack --out-format tiff

# nxs - nxs
python main.py -i ~/temp/nxs3 -D ~/temp/tiff_dark -F ~/temp/tiff_flat -o ~/temp/nxs4 --convert --data-as-stack --out-format nxs --in-format nxs

read -p "REMOVE CREATED DIRECTORIES FROM ~/temp/? ONLY CLICK YES IF ALL PASSED SUCCESSFULLY! [yes, N]: " -r
echo    # (optional) move to a new line
if [[ $REPLY =~ (yes)|(YES)|(Yes) ]]
then
    # we wanna be a bit more careful than rm -r ~/temp/f*
    rm -r ~/temp/[fts]*[0-9]*
    rm -r ~/temp/nxs[0-9]*
    rm -r ~/temp/tiff_flat
    rm -r ~/temp/tiff_dark
fi