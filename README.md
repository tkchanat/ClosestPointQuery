# Closest Point Query
|      bunny.obj      |      armadillo.obj      |      head.obj      |
| :-----------------: | :---------------------: | :----------------: |
| ![](docs/bunny.png) | ![](docs/armadillo.png) | ![](docs/head.png) |

## Benchmarks
Running on an Intel(R) Core(TM) i5-9400F, release build. All query points are generated randomly within a 1.5x unit sphere and 0.5 maximum query distance. 

| Model Name    | Triangles | Query Points | R-Tree Construct Time | Execution Time |
| :------------ | :-------- | :----------- | :-------------------- | :------------- |
| bunny.obj     | 5,002     | 100,000      | 0.008s                | 1.13s          |
| armadillo.obj | 212,574   | 100,000      | 0.503s                | 34.663s        |
| head.obj      | 1,131,776 | 100,000      | 2.791s                | 120.841s       |

Models downloaded from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)

## Build
This project uses third-party libraries as git submodules. Make sure to update and init them:
```sh
git submodule update --init
```

I'm using [premake5](https://premake.github.io/) for solution/project generation. The executable is already included, you may simply run:
```sh
premake5.exe vs2019
```
**Note:** If you're not using Visual Studio 2019, see [here](https://premake.github.io/docs/Using-Premake#using-premake-to-generate-project-files) for more options.

Now you can build the project with the generated project file.

## Development Log
- 2021-6-24
  - Gather libraries for kick starting the project.
  - Created `Mesh`, `Point`, `Triangle` class and successfully load an OBJ model.
- 2021-6-25 
  - Created a three.js visualizer for displaying results stored in CSV format.
  - Implemented the core algorithm for finding the closest point on a mesh.
- 2021-6-26
  - Using `std::async` and `std::future` to utilitize multi-core CPU computation.
  - Writing unit tests using GoogleTest. 
- 2021-6-27
  - Finalize solution structure, made an example project for benchmarking.
  - Conclude findings and documentations.

## Methodology
Finding the closest point on a given mesh is equivalent to breaking down the subproblem of finding the closest points to every single triangles. We have to manifest an efficient spatial structure for getting region of interest (ROI). I used an R-Tree approach to store triangles in a mesh. After searching for possible candidates within the search distance, iterate through all candidates and find the closest point from the query point to the triangle. 

To find the closest point to a triangle, for each triangle:

![](docs/triangle.png)
1. Calculate the normal of the triangle. (Right hand rule, counter-clockwise)
2. Find the projection from the query point onto the triangle's plane. 
3. Since the shortest possible length is the calculated orthogonal projection, we can rule out this triangle if the distance to plane is larger than the shortest distance we've found.
4. Else, the next closest point must be in one of these locations: lying on the edges (green), exactly on its vertices (yellow), or within the triangle itself (red).
5. To determine which case, we can check the number of edges it was considered "outside". This can be done by comparing the winding order with the different in direction of their cross products. 
6. Noted that a point can only be "outside" of at most two edges. And in that case, the closest point is on one of the vertices (green).
7. If the point is "outside" for only one edge, it obviously lies on the edge (yellow). Just make a projection on the edge and that's the closest point.
8. Or else, the projected point is already within the triangle itself. It's already the closest point on the triangle.
9. Repeat the above steps until all candidates are compared with the best closest point. 

## Assumption
- All faces must be triangulated.
- Currently only support querying multiple points on a single mesh.

## Possible Improvement
- To enable query on multiple meshes, we can use k-d tree to eliminate objects in a large scale.
- Use/develop a better SIMD mathematics library that supports platform/hardware acceleration.
- In the function `ClosestPointQuery::operator()`, a capturing lambda expression is used because it's bound by the RTree library. It's recommended to replace with a noraml function call to avoid overhead.
- Make an allocator and allocate a continuous memory for `Triangle` when constructing the RTree. Not only for a faster allocation, but also avoiding memory segmentation.
- Using R-Tree query restricted us from using axis-aligned box for searching triangles. This means we will have large gaps between the bounding box and the query sphere. Thus this will include triangles that aren't even close to the searching distance. 
- Implement the 2D method for calculating distance from a point to a triangle suggested by [this paper](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.104.4264&rep=rep1&type=pdf) by Mark W. Jones. By pre-computing matrices to transform triangles to align with axes and origin rather than performing vector maths. This method claiming to be 3-4 times faster than the 3D approach.
- Assembly analysis. (Ain't nobody got time for that)

## Dependencies
- [premake5](https://github.com/premake/premake-core) - for solution/project generation
- [glm](https://github.com/g-truc/glm) - for linear algebra calculation
- [googletest](https://github.com/google/googletest) - [prebuilt binary] for unit testing
- [RTree](https://github.com/nushoin/RTree) - for spatial query acceleration purpose
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) - for loading OBJ file
- [three.js](https://github.com/mrdoob/three.js/) - for visualizing the results