from __future__ import print_function
from is_wire.core import Channel, Subscription
from is_msgs.common_pb2 import Tensor
import numpy as np
import json
import sys
from plot_frames import plot_frame, make_figure

if len(sys.argv) < 3:
    print("USAGE: python consume.py <FROM> <HINTS> <TO>")
    sys.exit(0)

c = Channel(json.load(file("options.json"))["broker_uri"])
s = Subscription(c)

np.set_printoptions(precision=3, suppress=True)

axis = make_figure()
frame_throttle = 0 

topic = "FrameTransformation.%s" % ".".join(sys.argv[1:])
s.subscribe(topic)

while True:
    msg = c.consume()
    tf = msg.unpack(Tensor)
    T = np.matrix(tf.doubles).reshape(tf.shape.dims[0].size, tf.shape.dims[1].size)
    print(T, np.linalg.norm(T[:3,3]))

    if frame_throttle % 5 == 0:
    	plot_frame(T, axis)
    frame_throttle += 1	


