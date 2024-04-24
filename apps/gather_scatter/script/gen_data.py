import open3d as o3d
import numpy as np

def generate_kmap(point_cloud, k_neighbors):

    pcd = o3d.geometry.PointCloud()
    pcd.points = o3d.utility.Vector3dVector(point_cloud)
    
    search_tree = o3d.geometry.KDTreeFlann(pcd)
    
    kmap = []
    for i in range(len(point_cloud)):
        [_, idx, _] = search_tree.search_knn_vector_3d(pcd.points[i], k_neighbors + 1)
        kmap.append(idx[1:]) 

    return np.array(kmap)

point_cloud = np.random.rand(1000, 3)
k_neighbors = 5

kmap = generate_kmap(point_cloud, k_neighbors)
# print(kmap)

np.savetxt('kmap.txt', kmap, fmt='%d')

