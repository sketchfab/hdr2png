hdr2png
=======
#Dependencies:
sudo apt-get install libilmbase-dev libopenexr-dev libboost-regex-dev
src=$(pwd)

cd /sketchfab/tools/ && git clone https://github.com/sketchfab/oiio
cd oiio
make USE_TBB=false
#sudo cp -rf dist/linux*/* /usr/

#Tool compilation: cmake . -DOIIO_INCLUDE_DIR=<your oiio build/ dir>
export OIIO_DIR=/sketchfab/tools/oiio/dist/linux64
cd /sketchfab/tools/ && mkdir -p hdr2png && cmake ${src}/skfb_environments/tools/hdr2png/ && make
ln -s /sketchfab/tools/hdr2png/hdr2png ${src}/skfb_environments/tools/hdr2png/
