# TargetTimeTrialGame
![image](https://github.com/user-attachments/assets/2216b969-b378-4cc6-83c3-43be584b9481)

Leonardo Frias

Game music is original music created by me

# Description

A time trial style game in which the player is challenged to find all bunnies and shooting them with a projectile as fast as possible in order to stop the timer.

## Collisions

### Player to World(Structures)

Constants define the ymin y max and xmin xmax of the collision box for the player. At every call to ```player->move()``` a check is conducted through the creation of a candidate to determine whether the player's position is to be updated.

## Structures

Structures are generated with a call to Structure() which require a width a height to be specified and a cube mesh in order to generate the fracturing of the structure. The structure must be supplied a cube mesh, this is ensured with an assert(). The structures generate the meshes through instantiation.

### Drawing and generating fracturing

As to calculate collisions and generate the appropiate models for drawing, a strucuture is generated through the creation of n = width * height number of staticModelMatrix which are used to draw the cubes as well as to create a "physical" cube. After collision, the cube in the collision is popped from this vector and transformed into a freeCube and added to that vector, which no longer hold any collision information and are simply drawn with the instance VAO. 


## PBD Physics

Utilized to determine all physics based calculations for player gravity, debris (fractured cubes) and bullet trajectories.

## Collision system

Bullets have a collision radius that is used in order to fulfill the collision check and then proceeds to update the bullet's alive state to determine whether to "kill" the bullet removing it from the manager and removing its staticModelMatrix.

Once a bullet collides to a structure, it "kills" the bullet in order to stop drawing it and also pushes out cubes from the structure using the bullet's velocity vector to determine the projection/launch of the now "free cube." This removes the cube at the collision from the structure creating an opening for the collision system as the known cube from the wall is popped from the vector of cubes. 



# CONTROLS

wasd - move forward, left, backward, and right respectively

f/right click - shoot projectile

left click + drag - move camera

space - jump

r - enable ricochet mode

p - enable piercing mode

z/Z - zoom in/zoom out


# LIBS

- GLM library path env variable ```GLM_INCLUDE_DIR```

- GLFW library path env variable ```GLFW_DIR```

- GLEW library path env variable ```GLEW_DIR```

- Eigen library path (using Eigen 3) env variable ```EIGEN3_INCLUDE_DIR```

- SFML library path (using SFML 3 latest stable) env variable ```SFML3_DI```

- FreeType2 library path env variable (used for text rendering)

  ## On Linux

  For some distros (like openSUSE which was used in making this project) FLAC version may be to recent for SFML, a dirty fix is to:
```
ln {where you have your libs installed to}/libFLAC14.so {libs}/libFLAC12.so
```

# BUILDING

First clone the repository and then in the root directory of the project:
```
mkdir build && cd build && cmake .. && make -j4
```

# Citations

- Wall texture assets: https://cat-sopelka.itch.io/dungeon-brick-wall
