from __future__ import print_function
from is_wire.core import Channel, Subscription
from is_msgs.camera_pb2 import FrameTransformation
import numpy as np
import json
import sys

if len(sys.argv) < 3:
    print("USAGE: python consume.py <FROM> <HINTS> <TO>")
    sys.exit(0)

c = Channel(json.load(open("../etc/conf/options.json"))["broker_uri"])
s = Subscription(c)

np.set_printoptions(precision=3, suppress=True)

topic = "FrameTransformation.%s" % ".".join(sys.argv[1:])
s.subscribe(topic)

while True:
    message = c.consume()
    transformation = message.unpack(FrameTransformation) 
    tf = transformation.tf
    if len(tf.shape.dims):
        T = np.matrix(tf.doubles).reshape(tf.shape.dims[0].size, tf.shape.dims[1].size)
        print(T[:,3], np.linalg.norm(T[:3,3]))
