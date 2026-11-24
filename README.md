# finenav_demo

创建工作空间并拉取依赖仓库：

```bash
mkdir -p ~/ws_finenav_icra/src
cd ~/ws_finenav_icra/src

git clone https://github.com/FineNav/FineNav-Engine.git
git clone https://github.com/FineNav/Ndt_maps.git
git clone https://github.com/FineNav/finenav_map.git
git clone https://github.com/FineNav/finenav_tomography_planner.git
git clone https://github.com/FineNav/tomography_ndt_view.git
```

将本包放在 `~/ws_finenav_icra/src/finenav_demo`。当前本包尚未配置 Git 远程地址。

也可以使用本包的脚本一次拉取上述五个仓库，并在工作空间根目录执行
`colcon build --symlink-install`。运行前先加载 ROS 2 环境：

```bash
source /opt/ros/humble/setup.bash
bash ~/ws_finenav_icra/src/finenav_demo/clone.sh
```

默认克隆到 `finenav_demo` 的同级目录，已存在的目录会跳过。也可指定目标 `src` 目录：

```bash
bash ~/ws_finenav_icra/src/finenav_demo/clone.sh ~/another_workspace/src
```

手动编译时在工作空间根目录执行：

```bash
cd ~/ws_finenav_icra
colcon build --symlink-install
```

## 启动

构建完成并加载工作空间环境后，通过 launch 启动：

```bash
cd ~/ws_finenav_icra
source install/setup.bash
ros2 launch finenav_demo finenav_demo.launch.py
```

默认加载 `config/finenav_demo.yaml`，地图路径为
`/home/h1llo/Downloads/wheelchair.ot`，地图分辨率和规划网格均为 `0.05 m`。

指定其他参数文件：

```bash
ros2 launch finenav_demo finenav_demo.launch.py \
  params_file:=/path/to/params.yaml
```

指定其他地图及其分辨率：

```bash
ros2 launch finenav_demo finenav_demo.launch.py \
  map_file:=/path/to/map.ot map_resolution:=0.05
```

回放仿真时间数据时增加 `use_sim_time:=true`。
