#Hasan Asim 400512182

import numpy as np
import open3d as o3d

if __name__ == "__main__":
    scans = int(input("Enter number of scans performed: "))
    spins = 16  # Fixed number of angle steps per scan

    # Load point cloud data from file
    print("Loading point cloud data from file (XYZ format)...")
    pcd = o3d.io.read_point_cloud("point_array.xyz", format="xyz")

    # Output the raw point data to console
    print("Raw point cloud data:")
    print(np.asarray(pcd.points))

    # Display the point cloud in an Open3D window
    print("Opening point cloud visualizer...")
    o3d.visualization.draw_geometries([pcd])

    # Create a unique ID for each point in order
    yz_slice_vertex = []
    for x in range(0, scans * spins):
        yz_slice_vertex.append([x])

    # Connect points within each horizontal slice
    lines = []
    for x in range(0, scans * spins, spins):
        for i in range(spins):
            if i == spins - 1:
                lines.append([yz_slice_vertex[x + i], yz_slice_vertex[x]])  # connect last back to first
            else:
                lines.append([yz_slice_vertex[x + i], yz_slice_vertex[x + i + 1]])

    # Connect corresponding points between consecutive slices
    for x in range(0, scans * spins - spins - 1, spins):
        for i in range(spins):
            lines.append([yz_slice_vertex[x + i], yz_slice_vertex[x + i + spins]])

    # Build line set using point positions and line indices
    line_set = o3d.geometry.LineSet(
        points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),
        lines=o3d.utility.Vector2iVector(lines)
    )

    # Visualize point cloud with connecting lines
    o3d.visualization.draw_geometries([line_set])
