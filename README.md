# finenav_demo

## 下载与编译

mkdir -p ~/ws_finenav_icra/src
cd ~/ws_finenav_icra/src

git clone https://github.com/H1llo/finenav_icra_demo.git finenav_demo

source /opt/ros/humble/setup.bash
./finenav_demo/clone.sh


脚本自动克隆依赖仓库，并在工作空间根目录执行 `colcon build --symlink-install`。

## 启动

cd ~/ws_finenav_icra
source install/setup.bash
ros2 launch finenav_demo finenav_demo.launch.py

指定参数文件：

ros2 launch finenav_demo finenav_demo.launch.py \
  params_file:=/path/to/params.yaml

