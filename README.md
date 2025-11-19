### Build for Ubuntu
./build_ubuntu.sh
cd build
./genTopLut

Thay file cameraData.json trong thư mục data.
Thay kích thước mesh và file lut trong `main.h`.

const unsigned int IMG_WIDTH = 512;
const unsigned int IMG_HEIGHT = 320;
const float meshWidth = 16;
const float meshHeight = 18;

Dùng 3 kênh màu để xác định tọa độ xyz của pixcel
Red: X (0-255 tương ứng -meshHeight/2 đến meshHeight/2)
Green: Y (0-255 tương ứng -meshWidth/2 đến meshWidth/2)
Blue: 0 (bỏ qua); 255 (z = 0)

include: eigen_android, glm, json, libpng_android, opencv_android, opencv_x86