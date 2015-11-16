# cleanup after previous builds
TEMPDIR="./temp"
if [ -d "$TEMPDIR" ]; then
    rm -r "$TEMPDIR"
fi
mkdir "$TEMPDIR"

# compile extensions
cd ./src
python setup.py build_ext -b ../build -t ../temp
gcc -shared -fpic -lpthread -DSYSTEM=OPUNIX stplugin.c _st_shm.c -o ../build/_st_shm.plugin

# test the extension
cd ../test
python test_shm.py