#!/usr/bin/env python
import rospy
import roslib
import copy
from sensor_msgs.msg import Imu

class ImuFrameFixer:
    def __init__(self):
        input_topic = rospy.get_param('~input_topic')
        output_topic = rospy.get_param('~output_topic')
        self.new_frame_id = rospy.get_param('~new_frame_id')

        self.pub = rospy.Publisher(output_topic, Imu, queue_size=10)
        rospy.Subscriber(input_topic, Imu, self.callback)

    def callback(self, msg):
        msg.header.frame_id = self.new_frame_id
        self.pub.publish(msg)

if __name__ == '__main__':
    rospy.init_node('imu_frame_fixer')
    ImuFrameFixer()
    rospy.spin()
