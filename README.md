# Kimera-VIO-ROS

ROS Wrapper for [Kimera](https://github.com/MIT-SPARK/Kimera), fork by the Lunar Lab for baseline experiments.

## Installation

### Docker Setup

Make sure to install:
- [Docker](https://docs.docker.com/engine/install/ubuntu/)

Then, setup this repository in a ROS1 workspace at a desired location on your computer.

After that, navigate to the `docker` directory. Log in to the user that you want the docker file to create in the container. Then, edit the `DOCKERFILE` to update these lines:
- `ARG USERNAME=`: Your username
- `ARG USER_UID=`: Output of `echo $UID`
- `ARG USER_GID=`: Output of `id -g`

Edit the `enter_container.sh` script with the following paths:
- `DATA_DIR=`: The directory where the any datasets are located
- `ROS_WS_DIR=`: The directory of the ros workspace this repository is a part of

Now, run the following commands:
```
build_container.sh
run_container.sh
```

The rest of this README **assumes that you are inside the Docker container**. For easier debugging and use, its highly recommended to install the [VSCode Docker extension](https://code.visualstudio.com/docs/containers/overview), which allows you to start/stop the container and additionally attach VSCode to the container by right-clicking on the container and selecting `Attach Visual Studio Code`.

### KimeraVIO ROS Install

Next navigate to the root of your ROS workspace, and run the following commands:
```bash
source /opt/ros/noetic/setup.bash
caktin init
catkin config --cmake-args -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wno-error" -DGTSAM_TANGENT_PREINTEGRATION=OFF
catkin config --merge-devel
echo 'source ~/<name_of_ros_ws>/devel/setup.bash' >> ~/.bashrc
```
Now, navigate into the `src` folder and then run this command: 
```bash
wstool init
```

Depending on if you want to downlaod other dependencies via ssh or https, run one of the following commands:
```bash
wstool merge Kimera-VIO-ROS/install/kimera_vio_ros_ssh.rosinstall # ssh
```
```bash
wstool merge Kimera-VIO-ROS/install/kimera_vio_ros_https.rosinstall # https
```

Now run the following:
```bash
wstool update
rosdep update
rosdep install --from-paths . --ignore-src -r -y
```

Finally, navigate back up to the root ROS workspace folder and compile the code:
```bash
catkin build
source ~/<name_of_ros_ws>/devel/setup.bash
```

## Usage
Download a [Euroc](https://projects.asl.ethz.ch/datasets/doku.php?id=kmavvisualinertialdatasets) rosbag: for example [V1_01_easy](http://robotics.ethz.ch/~asl-datasets/ijrr_euroc_mav_dataset/vicon_room1/V1_01_easy/V1_01_easy.bag).

### Online
  1. As a general good practice, open a new terminal and run: `roscore`

  2. In another terminal, launch KimeraVIO ROS wrapper:
  ```bash
  roslaunch kimera_vio_ros kimera_vio_ros_euroc.launch
  ```

  3. In another terminal, launch rviz for visualization:
  ```bash
  rviz -d $(rospack find kimera_vio_ros)/rviz/kimera_vio_euroc.rviz
  ```
  > Note: this rviz configuration makes use of a rviz plugin: [mesh_rviz_plugins](https://github.com/MIT-SPARK/mesh_rviz_plugins). To visualize the textured 3D mesh, clone this plugin to your catkin workspace and catkin build it (note that this should be done automatically via `wstool`).

  4. Finally, in another terminal, launch the downloaded Euroc rosbag:
  ```bash
  rosbag play --clock /PATH/TO/EUROC_ROSBAG
  ```

  > Note that you will need to both source ROS and your `catkin_ws` for each new terminal unless you added the following lines to your `~/.bashrc` file:
  > ```bash
  > source /opt/ros/noetic/setup.bash  # Change `noetic` for your ROS distribution.
  > source ~/catkin_ws/devel/setup.bash # Change `bash` to the shell you use.
  > ```

### Offline
  In this mode, the provided rosbag will be first parsed and then sent to the VIO for processing.
  This is particularly useful when debugging to avoid potential ROS networking issues.

  First, edit `Kimera-VIO/params/Euroc/BackendParams.yaml` so that `autoInitialize` is set to `1`.

  Next, edit the environment variables in the `tmux/euroc_example.yaml` file and then run the following command:
  ```bash
  tmuxp load tmux/euroc_example.yaml
  ```

## Other functionalities

### Using camera_info topics instead of Yaml parameters

It is sometimes convenient to use the `camera_info` topics to parse the camera's parameters.
There is currently only one way to use these topics:
 - Offline: using the launch file `launch/cam_info_yamlizer.launch` which will generate yaml files out of the topics.
 You need to make sure that the `frame_id`s and the ROS topics are correctly set. Also, mind that the left/right cam frame ids are typically set as static tfs in a rosbag, therefore, first launch the node, and then run the rosbag (in case you see an exception bcs of a missing frame_id).

### Restart Kimera-VIO

The typical use case is that you have multiple rosbags and you don't want to be killing Kimera-VIO(-ROS) each time.
If this is your case, then we provide a rosservice to restart Kimera-VIO (it will do a hard restart, meaning the whole pipeline and data provider will be destructed and constructed again).
```bash
rosservice call /kimera_vio_ros/kimera_vio_ros_node/restart_kimera_vio
```
> Note that Kimera-VIO will complain if timestamps are not strictly increasing. Therefore, one must follow these steps:
> 1. Start Kimera-VIO and rosbag
> 2. Stop rosbag
> 3. Call rosservice to restart VIO
> 4. Start another rosbag

### Enable Dense Depth Stereo estimation

This will run OpenCV's StereoBM algorithm, more info can be found [here](http://wiki.ros.org/stereo_image_proc) (also checkout this to [choose good parameters](http://wiki.ros.org/stereo_image_proc/Tutorials/ChoosingGoodStereoParameters)):

```bash
roslaunch kimera_vio_ros kimera_vio_ros_euroc run_stereo_dense:=1
```

This will publish a `/stereo_gray/points2` topic, which you can visualize in Rviz as a 3D pointcloud.
Alternatively, if you want to visualize the depth image, since Rviz does not provide a plugin to
visualize a [disparity image](http://docs.ros.org/api/stereo_msgs/html/msg/DisparityImage.html), we also run a [disparity_image_proc](https://github.com/MIT-SPARK/disparity_image_proc) nodelet that will publish the depth image to `/stereo_gray/disparity_image_proc/depth/image_raw`.

## Hardware use

See the [documentation on hardware setup](docs/hardware_setup.md) for instructions on running KimeraROS on supported hardware platforms, as well as guides on how to develop for other platforms.

## BSD License
KimeraVIO ROS wrapper is open source under the BSD license, see the [LICENSE.BSD](./LICENSE.BSD) file.
