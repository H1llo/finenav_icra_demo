# finenav_demo

## 下载与编译

```bash
mkdir -p ~/ws_finenav_icra/src
cd ~/ws_finenav_icra/src
git clone https://github.com/H1llo/finenav_icra_demo.git finenav_demo
source /opt/ros/humble/setup.bash
bash finenav_demo/clone.sh
cd ~/ws_finenav_icra
source install/setup.bash
colcon build --symlink-install --cmake-args -DTOMOGRAPHY_NDT_WITH_FINENAV=ON
```

脚本自动克隆依赖并编译，最后一条命令启用 WorldView 插件注册。

## 启动

```bash
cd ~/ws_finenav_icra
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch finenav_demo finenav_demo.launch.py
```

加载源码目录中的参数文件：

```bash
cd ~/ws_finenav_icra
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch finenav_demo finenav_demo.launch.py params_file:="$PWD/src/finenav_demo/config/finenav_demo.yaml"
```
