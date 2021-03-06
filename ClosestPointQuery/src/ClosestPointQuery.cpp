#include "ClosestPointQuery.h"

namespace geoutils {

	ClosestPointQuery::ClosestPointQuery(const Mesh& m) {
		// Construct the R-Tree
		const size_t triangle_count = m.indices.size() / 3;
		triangles.reserve(triangle_count);
		for (size_t i = 0; i < m.indices.size(); i += 3) {
			const auto& p1 = m.vertices[m.indices[i + 0]];
			const auto& p2 = m.vertices[m.indices[i + 1]];
			const auto& p3 = m.vertices[m.indices[i + 2]];
			triangles.emplace_back(p1, p2, p3);
			const Vec3 min = p1.min(p2).min(p3);
			const Vec3 max = p1.max(p2).max(p3);
			r_star_tree.insert(min, max, &triangles.back());
		}
	}

	bool ClosestPointQuery::operator() (const Point& query_point, float max_dist, Point& closest_point) const {
		// First, get the bounding box of the radius positioned at query point.
		double shortest_distance = DBL_MAX;
		const Point search_min(query_point - Vec3(max_dist));
		const Point search_max(query_point + Vec3(max_dist));
		const auto search_callback = [&](Triangle* tri) -> bool {
			uint8_t outside_count = 0;
			// Determine the triangle normal and projected point.
			const auto& vert = tri->vertices;
			const Vec3 normal = (vert[1] - vert[0]).cross(vert[2] - vert[0]).normalize();
			const Vec3 projection = normal * (vert[0] - query_point).dot(normal);
			const double distance_to_plane = projection.length2();

			// Early termination. (distance_to_plane is already the shortest possible distance to the triangle, there's no reason to proceed)
			if (distance_to_plane > shortest_distance) return true;

			const Point projected = query_point + projection;
			for (uint8_t i = 0; i < 3; ++i) {
				const Point& v1 = vert[i];
				const Point& v2 = vert[(i + 1) % 3];

				// Utilize the winding order to determine if the point lies outside of an edge.
				const bool outside = (v1 - projected).cross(v2 - projected).dot(normal) < 0.f;
				if (outside) {
					outside_count++;
					// Clamp the projection value to be in-between of the two ends of the edge.
					const float t = std::min(std::max((v2 - v1).dot(projected - v1) / v1.distance2(v2), 0.f), 1.f);
					const Point closest_point_on_edge = v1 * (1.f - t) + v2 * t;
					const double distance_to_edge = query_point.distance2(closest_point_on_edge);
					if (distance_to_edge < shortest_distance) {
						closest_point = closest_point_on_edge;
						shortest_distance = distance_to_edge;
					}
				}

				// Early termination. (A point can only be outside of at most 2 edges)
				if (outside_count > 1) return true;
			}

			// Projection of the query point lies within the triangle.
			if (outside_count == 0) {
				closest_point = projected;
				shortest_distance = distance_to_plane;
			}
			return true; // Keep traversing
		};

		// Query the R-Tree around the bounding box which we defined eariler.
		// For each overlapping triangles, find the closest point from the query point to the triangle.
		// A detailed explanation can be found in README.md.
		r_star_tree.search_radius(
			query_point,
			max_dist,
			search_callback
		);

		return shortest_distance != DBL_MAX; // Return true if the closest point is found, else false.
	}

} // namespace geoutils