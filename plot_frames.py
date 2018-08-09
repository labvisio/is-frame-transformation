import numpy as np 
import matplotlib.pyplot as plt 
from mpl_toolkits.mplot3d import Axes3D

#a=np.matrix([[-0.398,  0.051,  0.916, -0.021],
# [-0.894,  0.201, -0.4  , -0.   ],
# [-0.205, -0.978, -0.034,  1.175],
# [ 0.   ,  0.   ,  0.   ,  1.   ]])
    #'0.807126879692 -0.520674049854 -0.278289377689 0.00147371739149; 0.482435017824 0.853386104107 -0.19745542109 0.00950167234987;0.340298175812 0.0251150391996 0.939982235432 1.17362141609; 0 0 0 1')

def make_figure():
  plt.ion()
  fig = plt.figure()
  # Create a 3D figure
  ax = fig.add_subplot(111, projection='3d')
  return ax 

def plot_frame(a, ax):
  ax.clear()

  # Create the robot frame
  u_vectors = np.array([1,0,0])
  v_vectors = np.array([0,1,0])
  w_vectors = np.array([0,0,1])
  posx = np.array([0,0,0])
  posy = np.array([0,0,0])
  posz = np.array([0,0,0])
  
  
  # Separate the position and orientation of the camera  frame and axis
  u_cam_vec=np.array(a[0,0:3])
  v_cam_vec=np.array(a[1,0:3])
  w_cam_vec=np.array(a[2,0:3])
  cam_posx=np.array([a[0,3], a[0,3], a[0,3]])
  cam_posy=np.array([a[1,3], a[1,3], a[1,3]])
  cam_posz=np.array([a[2,3], a[2,3], a[2,3]])
  
  
  # plot the robot base frame and label the axis 
  ax.quiver(posx,posy,posz,u_vectors,v_vectors,w_vectors)
  ax.text(1,0,0,"X")
  ax.text(0,1,0,"Y")
  ax.text(0,0,1,"Z")
  ax.text(0,0,0,"Robot")
  
  
  # plot the camera frame and label the axis
  ax.quiver(cam_posx,cam_posy,cam_posz,u_cam_vec,v_cam_vec,w_cam_vec)
  ax.text((a[0,3]+a[0,0]),(a[1,3]+a[1,0]),(a[2,3]+a[2,0]),"X")
  ax.text((a[0,3]+a[0,1]),(a[1,3]+a[1,1]),(a[2,3]+a[2,1]),"Y")
  ax.text((a[0,3]+a[0,2]),(a[1,3]+a[1,2]),(a[2,3]+a[2,2]),"Z")
  ax.text(a[0,3],a[1,3],a[2,3],"Camera")
  
  ax.axis('square')
  ax.set_xlim(-2.5,2.5)
  ax.set_ylim(-2.5,2.5)
  ax.set_zlim(-2.5,2.5) 
  ax.set_xlabel('X axis')
  ax.set_ylabel('Y axis')
  ax.set_zlabel('Z axis')
  
  #plt.axis('equal')
  plt.pause(0.00005) 
  plt.show(block=False)
