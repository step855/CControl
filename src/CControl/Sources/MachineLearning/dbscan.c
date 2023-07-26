/*
 * dbscan.c
 *
 *  Created on: 22 juli 2023
 *      Author: Daniel Mårtensson
 */

#include "../../Headers/functions.h"

/* 
 * Create ID's of clusters
 * A[m*n]
 * idx[m]
 * epsilon = Raduis of the clusters
 * min_pts = Minimum points of a valid cluster
 */
void dbscan(float A[], size_t idx[], float epsilon, size_t min_pts, size_t row, size_t column) {
	/* Create idx */
	memset(idx, 0, row * sizeof(size_t));

	/* Create pdist2 C */
	float* C = (float*)malloc(row * row * sizeof(float));
	pdist2(A, A, C, row, column, row, PDIST2_METRIC_L2);

	/* Flags */
	bool* visited = (bool*)malloc(row * sizeof(bool));
	memset(visited, false, row * sizeof(bool));

	/* Lists */
	int32_t* neighbors = (int32_t*)malloc(row * sizeof(int32_t));
	int32_t* neighbors2 = (int32_t*)malloc(row * sizeof(int32_t));

	/* Process */
	size_t n1, n2, n3, i, j, k, l = 0, m;
	for (i = 0; i < row; i++) {
		if (!visited[i]) {
			visited[i] = true;
			find(C + i * row, neighbors, epsilon, row, FIND_CONDITION_METOD_GE);

			/* Count number of elements */
			n1 = 0;
			for (m = 0; m < row; m++) {
				if (neighbors[m] >= 0) {
					n1++;
				}
				else {
					break;
				}
			}

			/* Create */
			int32_t* neighbors1 = (int32_t*)malloc(n1 * sizeof(int32_t));
			memcpy(neighbors1, neighbors, n1 * sizeof(int32_t));

			if (n1 > min_pts) {
				l++;
				idx[i] = l;

				k = 0;
				while (true) {
					j = neighbors1[k];
					if (!visited[j]) {
						visited[j] = true;
						find(C + j * row, neighbors2, epsilon, row, FIND_CONDITION_METOD_GE);
						
						/* Count number of elements */
						n2 = 0;
						for (m = 0; m < row; m++) {
							if (neighbors2[m] >= 0) {
								n2++;
							}
							else {
								break;
							}
						}

						/* MATLAB: neighbors1 = [neighbors1 neighbors2] */
						if (n2 >= min_pts) {
							neighbors1 = (int32_t*)realloc(neighbors1, (n1 + n2) * sizeof(int32_t));
							for (m = 0; m < n2; m++) {
								neighbors1[n1 + m] = neighbors2[m];
							}
							n1 += n2;
						}
					}

					if (idx[j] == 0) {
						idx[j] = l;
					}
					k++;
					n3 = 0;
					for (m = 0; m < n1; m++) {
						if (neighbors1[m] >= 0) {
							n3++;
						}
						else {
							break;
						}
					}
					if (k >= n3) {
						break;
					}
				}
			}

			/* Free */
			free(neighbors1);
		}
	}

	/* Free */
	free(C);
	free(visited);
	free(neighbors);
	free(neighbors2);
}

/* 
 * GNU Octave code
	% Density-based spatial clustering of applications with noise
	% Input: X(data), epsilon(Radius), min_pts(Minimum points)
	% Output: idx(row index of class ID)
	% Example 1: [idx] = mi.dbscan(X, espsilon, min_pts)
	% Author: Daniel Mårtensson, Juli 2023

	function [idx] = dbscan(X, epsilon, min_pts)
	  A = 0;
	  n = size(X,1);
	  idx = zeros(n,1);
	  D = sqrt(distEucSq(X, X));
	  visited = false(n, 1);
	  for i=1:n
		if ~visited(i)
		  visited(i) = true;
		  neighbors1 = find(epsilon >= D(i,:));
		  n1 = numel(neighbors1);
		  if n1 > min_pts
			A = A + 1;
			idx(i) = A;
			k = 1;
			while true
			  j = neighbors1(k);
			  if ~visited(j)
				visited(j) = true;
				neighbors2 = find(epsilon >= D(j,:));
				n2 = numel(neighbors2);
				if n2 >= min_pts
				  neighbors1 = [neighbors1 neighbors2];
				end
			  end
			  if idx(j) == 0
				idx(j) = A;
			  end
			  k = k + 1;
			  if k > numel(neighbors1)
				break;
			  end
			end
		  end
		end
	  end
	end

	function D = distEucSq(X, Y)
	  Yt = Y';
	  XX = sum(X.*X,2);
	  YY = sum(Yt.*Yt,1);
	  D = XX + YY - 2*X*Yt;
	end
 */