find . -name "*.so" -exec patchelf --set-rpath . {} \;
find . -name "*Test" -exec patchelf --set-rpath . {} \;
find . -name "MantidPlot" -exec patchelf --set-rpath . {} \;
find . -name "WikiMaker" -exec patchelf --set-rpath . {} \;


