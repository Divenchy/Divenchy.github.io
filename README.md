# TargetTimeTrialGame
![image](https://github.com/user-attachments/assets/2216b969-b378-4cc6-83c3-43be584b9481)

Leonardo Frias

Game music original music created by me


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
