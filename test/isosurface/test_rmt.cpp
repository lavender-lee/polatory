// Copyright (c) 2016, GSI and The Polatory Authors.

#include <array>
#include <cmath>

#include <gtest/gtest.h>

#include <polatory/common/eigen_utility.hpp>
#include <polatory/geometry/bbox3d.hpp>
#include <polatory/geometry/point3d.hpp>
#include <polatory/isosurface/rmt_lattice.hpp>
#include <polatory/point_cloud/random_points.hpp>

using polatory::common::row_range;
using polatory::geometry::bbox3d;
using polatory::geometry::cuboid3d;
using polatory::geometry::point3d;
using polatory::geometry::vector3d;
using polatory::isosurface::edge_bitset;
using polatory::isosurface::edge_index;
using polatory::isosurface::FaceEdges;
using polatory::isosurface::NeighborCellVectors;
using polatory::isosurface::NeighborEdgePairs;
using polatory::isosurface::NeighborMasks;
using polatory::isosurface::OppositeEdge;
using polatory::isosurface::PrimitiveVectors;
using polatory::isosurface::ReciprocalPrimitiveVectors;
using polatory::isosurface::rmt_primitive_lattice;
using polatory::isosurface::rotation;
using polatory::point_cloud::random_points;

// Relative positions of neighbor nodes connected by each edge.
std::array<vector3d, 14> NeighborVectors
  {
    rotation().transform_vector({ +1., +1., +1. }),
    rotation().transform_vector({ +2., +0., +0. }),
    rotation().transform_vector({ +1., -1., -1. }),
    rotation().transform_vector({ +0., +2., +0. }),
    rotation().transform_vector({ +1., +1., -1. }),
    rotation().transform_vector({ +0., +0., -2. }),
    rotation().transform_vector({ -1., +1., -1. }),
    rotation().transform_vector({ -1., -1., -1. }),
    rotation().transform_vector({ -2., +0., +0. }),
    rotation().transform_vector({ -1., +1., +1. }),
    rotation().transform_vector({ +0., -2., +0. }),
    rotation().transform_vector({ -1., -1., +1. }),
    rotation().transform_vector({ +0., +0., +2. }),
    rotation().transform_vector({ +1., -1., +1. })
  };

TEST(rmt, face_edges) {
  for (edge_bitset edge_set : FaceEdges) {
    auto e0 = polatory::isosurface::bit::pop(&edge_set);
    auto e1 = polatory::isosurface::bit::pop(&edge_set);
    auto e2 = polatory::isosurface::bit::pop(&edge_set);

    auto& v0 = NeighborVectors[e0];
    auto& v1 = NeighborVectors[e1];
    auto& v2 = NeighborVectors[e2];

    auto area = (v1 - v0).cross(v2 - v0).norm();
    ASSERT_DOUBLE_EQ(2.0 * std::sqrt(2.0), area);
  }
}

TEST(rmt, lattice) {
  point3d min(-1.0, -1.0, -1.0);
  point3d max(1.0, 1.0, 1.0);

  bbox3d bbox(min, max);
  double resolution = 0.01;

  rmt_primitive_lattice lat(bbox, resolution);

  auto points = random_points(cuboid3d(min, max), 100);

  for (auto p : row_range(points)) {
    auto ci = lat.cell_contains_point(p);
    auto cv = lat.cell_vector_from_index(ci);
    auto cp = lat.point_from_cell_vector(cv);

    ASSERT_LT((p - cp).norm(), std::sqrt(2.0) * resolution);
  }
}

TEST(rmt, neighbor_edge_pairs) {
  for (edge_index ei = 0; ei < 14; ei++) {
    auto& va = NeighborVectors[ei];

    for (auto& pair : NeighborEdgePairs[ei]) {
      auto& vb = NeighborVectors[pair.first];
      auto& vc = NeighborVectors[pair.second];

      ASSERT_TRUE(va.dot(vb) > 0.0);
      ASSERT_TRUE(va.dot(vc) > 0.0);
      ASSERT_NEAR(0.0, vb.cross(va).cross(va.cross(vc)).norm(), 1e-15);
    }
  }
}

TEST(rmt, neighbors) {
  for (size_t i = 0; i < NeighborMasks.size(); i++) {
    auto mask = NeighborMasks[i];
    auto count = polatory::isosurface::bit::count(mask);
    ASSERT_TRUE(count == 4 || count == 6);

    auto vi = NeighborVectors[i];
    for (int k = 0; k < count; k++) {
      auto j = polatory::isosurface::bit::pop(&mask);
      auto vj = NeighborVectors[j];
      double vijsq = (vj - vi).squaredNorm();
      if (vijsq > 3.5) {
        ASSERT_DOUBLE_EQ(4.0, vijsq);
      } else {
        ASSERT_DOUBLE_EQ(3.0, vijsq);
      }
    }
  }

  for (edge_index ei = 0; ei < 14; ei++) {
    vector3d computed =
      PrimitiveVectors[0] * NeighborCellVectors[ei][0]
      + PrimitiveVectors[1] * NeighborCellVectors[ei][1]
      + PrimitiveVectors[2] * NeighborCellVectors[ei][2];

    for (int i = 0; i < 3; i++) {
      ASSERT_NEAR(NeighborVectors[ei](i), computed(i), 1e-15);
    }
  }
}

TEST(rmt, opposite_edge) {
  for (edge_index ei = 0; ei < 14; ei++) {
    ASSERT_EQ(-NeighborVectors[ei], NeighborVectors[OppositeEdge[ei]]);
  }
}

TEST(rmt, primitive_vectors) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      auto dot = PrimitiveVectors[i].dot(ReciprocalPrimitiveVectors[j]);
      if (i == j) {
        ASSERT_NEAR(1.0, dot, 1e-15);
      } else {
        ASSERT_NEAR(0.0, dot, 1e-15);
      }
    }
  }
}
