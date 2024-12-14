#include "TerrainGen.h"

void TerrainGen::add_conn(TSharedPtr<Node> node1, TSharedPtr<Node> node2)
{
	if (node1->get_point() == node2->get_point())
	{
		return;
	}

	if (!node1->get_next_point(node2->get_node()).IsSet())
	{
		node1->add_connection(node2);
	}
	if (!node2->get_next_point(node1->get_node()).IsSet())
	{
		node2->add_connection(node1);
	}
}
TSharedPtr<Node> TerrainGen::insert_conn(TSharedPtr<Node> node1_to_insert, TSharedPtr<Node> node2_to_insert,
										 FVector node3_point)
{
	for (int i = 0; i < node1_to_insert->conn.Num(); i++)
	{
		if (node1_to_insert->conn[i]->node == node2_to_insert)
		{
			node1_to_insert->conn.RemoveAt(i);
			break;
		}
	}
	for (int i = 0; i < node2_to_insert->conn.Num(); i++)
	{
		if (node2_to_insert->conn[i]->node == node1_to_insert)
		{
			node2_to_insert->conn.RemoveAt(i);
			break;
		}
	}
	TSharedPtr<Node> new_node(MakeShared<Node>(node3_point));
	add_conn(node1_to_insert, new_node);
	add_conn(node2_to_insert, new_node);
	return new_node;
}

void TerrainGen::move_river(TSharedPtr<Node>& node1, TSharedPtr<Node>& node2)
{
	if (node1->is_used() || node2->is_used())
	{
		return;
	}

	FVector point1 = node1->get_point();
	FVector backup_point1 = node1->get_point();
	point_shift(point1);
	FVector point2 = node2->get_point();
	FVector backup_point2 = node2->get_point();
	point_shift(point2);

	for (auto conn : node1->conn)
	{
		double dist = FVector::Distance(conn->node->get_point(), point1);
		if (dist > max_river_length / 3 * 2 || dist < av_river_length / 3 * 2)
		{
			return;
		}
	}
	for (auto conn : node2->conn)
	{
		double dist = FVector::Distance(conn->node->get_point(), point2);
		if (dist > max_river_length || dist < av_river_length / 3 * 2)
		{
			return;
		}
	}
	if (
		// FVector::Distance(point1, point2) > max_river_length / 3 * 2 ||
		FVector::Distance(point1, point2) < av_river_length / 3 * 2)
	{
		return;
	}

	if (!AllGeometry::is_intersect_array(backup_point1, point1, river, false).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point1, point1, map_borders_array, true).IsSet())
	{
		node1->set_point(point1);
		// return;
	}
	if (!AllGeometry::is_intersect_array(backup_point2, point2, river, false).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point2, point2, map_borders_array, true).IsSet())
	{
		node2->set_point(point2);
		// return;
	}
}

void TerrainGen::move_road(TSharedPtr<Node>& node)
{
	if (node->is_used())
	{
		return;
	}

	if (node->conn.Num() == 2)
	{
		if (!AllGeometry::is_intersect_array(node->conn[0]->node->get_point(), node->conn[1]->node->get_point(), river,
											 true)
				 .IsSet() &&
			!AllGeometry::is_intersect_array(node->conn[0]->node->get_point(), node->conn[1]->node->get_point(),
											 map_borders_array, true)
				 .IsSet() &&
			!AllGeometry::is_intersect_array(node->conn[0]->node->get_point(), node->conn[1]->node->get_point(), roads,
											 false)
				 .IsSet())
		{
			node->set_point((node->conn[0]->node->get_point() + node->conn[1]->node->get_point()) / 2);
			// return;
		}
	}

	FVector point = node->get_point();
	FVector backup_point = node->get_point();
	point_shift(point);

	for (auto conn : node->conn)
	{
		if (FVector::Distance(conn->node->get_point(), point) > max_road_length)
		{
			return;
		}
	}

	if (!AllGeometry::is_intersect_array(backup_point, point, river, true).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point, point, map_borders_array, true).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point, point, roads, false).IsSet())
	{
		node->set_point(point);
		return;
	}
}

void TerrainGen::create_terrain(TArray<TSharedPtr<Node>>& roads_, TArray<District>& figures_array_,
								District& river_figure_, TArray<TSharedPtr<Node>>& map_borders_array_,
								TArray<FVector>& debug_points_array_)
{
	river.Empty();
	roads.Empty();
	road_centers.Empty();
	map_points_array.Empty();
	map_borders_array.Empty();
	guididng_roads_array.Empty();
	weighted_points.Empty();
	figures_array.Empty();

	auto map_node1 = MakeShared<Node>(0, 0, 0);
	auto map_node2 = MakeShared<Node>(0, y_size, 0);
	auto map_node3 = MakeShared<Node>(x_size, y_size, 0);
	auto map_node4 = MakeShared<Node>(x_size, 0, 0);
	add_conn(map_node1, map_node2);
	add_conn(map_node2, map_node3);
	add_conn(map_node3, map_node4);
	add_conn(map_node4, map_node1);
	map_borders_array.Add(map_node4);
	map_borders_array.Add(map_node3);
	map_borders_array.Add(map_node2);
	map_borders_array.Add(map_node1);
	double points_count = 64;
	double av_size = (x_size + y_size) / 2;

	weighted_points.Empty();

	const double point_row_counter = sqrt(points_count);
	for (double x = 1; x < point_row_counter; x++)
	{
		for (int y = 1; y < point_row_counter; y++)
		{
			weighted_points.Add(WeightedPoint{
				FVector(static_cast<int>(av_size / point_row_counter * x +
										 (rand() % (static_cast<int>(av_size / point_row_counter / 2))) -
										 (av_size / point_row_counter / 4)),
						static_cast<int>(av_size / point_row_counter * y) +
							(rand() % (static_cast<int>(av_size / point_row_counter / 2))) -
							(av_size / point_row_counter / 4),
						0),
				(rand() % static_cast<int>(av_size / point_row_counter / 2)) + (av_size / point_row_counter / 3)});
		}
	}

	double StartTime1 = FPlatformTime::Seconds();
	create_guiding_rivers();
	double EndTime1 = FPlatformTime::Seconds();
	double StartTime3 = FPlatformTime::Seconds();
	get_river_figure();
	for (int iter = 0; iter < 100; iter++)
	{
		for (auto& b : bridges)
		{
			move_river(b.Key, b.Value);
		}
	}
	process_bridges();

	double EndTime3 = FPlatformTime::Seconds();
	double StartTime4 = FPlatformTime::Seconds();
	create_guiding_roads();
	double EndTime4 = FPlatformTime::Seconds();
	double StartTime5 = FPlatformTime::Seconds();
	for (int iter = 0; iter < 10; iter++)
	{
		for (auto& r : roads)
		{
			r->set_used(false);
		}

		for (auto& road_center : road_centers)
		{
			road_center->set_used(true);
		}

		for (auto& r : roads)
		{
			move_road(r);
		}
	}
	double EndTime5 = FPlatformTime::Seconds();
	double StartTime6 = FPlatformTime::Seconds();
	int old_nodes = 0;
	while (roads.Num() != old_nodes)
	{
		old_nodes = roads.Num();
		create_usual_roads();
		if (roads.Num() > 2500)
		{
			break;
		}
	}
	double EndTime6 = FPlatformTime::Seconds();
	double StartTime7 = FPlatformTime::Seconds();
	for (int i = 0; i < 2; i++)
	{
		for (auto& r : roads)
		{
			r->set_used(false);
		}

		for (auto& road_center : road_centers)
		{
			road_center->set_used(true);
		}
		for (auto& r : roads)
		{
			move_road(r);
		}
	}
	// TSharedPtr<Node> node1 = MakeShared<Node>(1000, 1000, 0, 1);
	// TSharedPtr<Node> node2 = MakeShared<Node>(2000, 1000, 0, 2);
	// TSharedPtr<Node> node3 = MakeShared<Node>(2000, 2000, 0, 3);
	// TSharedPtr<Node> node4 = MakeShared<Node>(1000, 2000, 0, 4);
	// TSharedPtr<Node> node5 = MakeShared<Node>(1500, 1700, 0, 5);
	// TSharedPtr<Node> node6 = MakeShared<Node>(1500, 800, 0, 5);
	// TSharedPtr<Node> node1 = MakeShared<Node>(0.000, 1960.000, 0);
	// TSharedPtr<Node> node2 = MakeShared<Node>(164.373, 1994.857, 0);
	// TSharedPtr<Node> node3 = MakeShared<Node>(82.187, 1977.429, 0);
	// TSharedPtr<Node> node4 = MakeShared<Node>(246.560, 2012.286, 0);
	// TSharedPtr<Node> node5 = MakeShared<Node>(295.586, 2033.505, 0);
	// TSharedPtr<Node> node6 = MakeShared<Node>(380.564, 2055.432, 0);
	// TSharedPtr<Node> node7 = MakeShared<Node>(478.542, 2075.239, 0);
	// TSharedPtr<Node> node8 = MakeShared<Node>(574.115, 2102.578, 0);
	// TSharedPtr<Node> node9 = MakeShared<Node>(671.394, 2117.085, 0);
	// TSharedPtr<Node> node10 = MakeShared<Node>(738.247, 2133.047, 0);
	// TSharedPtr<Node> node11 = MakeShared<Node>(817.780, 2146.081, 0);
	// TSharedPtr<Node> node12 = MakeShared<Node>(898.744, 2138.277, 0);
	// TSharedPtr<Node> node13 = MakeShared<Node>(988.615, 2123.589, 0);
	// TSharedPtr<Node> node14 = MakeShared<Node>(1085.368, 2147.567, 0);
	// TSharedPtr<Node> node15 = MakeShared<Node>(1183.306, 2166.981, 0);
	// TSharedPtr<Node> node16 = MakeShared<Node>(1265.614, 2198.283, 0);
	// TSharedPtr<Node> node17 = MakeShared<Node>(1308.204, 2242.366, 0);
	// TSharedPtr<Node> node18 = MakeShared<Node>(1382.228, 2268.474, 0);
	// TSharedPtr<Node> node19 = MakeShared<Node>(1471.647, 2285.639, 0);
	// TSharedPtr<Node> node20 = MakeShared<Node>(1559.949, 2300.519, 0);
	// TSharedPtr<Node> node21 = MakeShared<Node>(1646.761, 2321.838, 0);
	// TSharedPtr<Node> node22 = MakeShared<Node>(1735.269, 2342.497, 0);
	// TSharedPtr<Node> node23 = MakeShared<Node>(1816.095, 2356.922, 0);
	// TSharedPtr<Node> node24 = MakeShared<Node>(1844.711, 2418.158, 0);
	// TSharedPtr<Node> node25 = MakeShared<Node>(1881.314, 2492.885, 0);
	// TSharedPtr<Node> node26 = MakeShared<Node>(1917.917, 2567.612, 0);
	// TSharedPtr<Node> node27 = MakeShared<Node>(1932.504, 2597.392, 0);
	// TSharedPtr<Node> node28 = MakeShared<Node>(1890.476, 2545.029, 0);
	// TSharedPtr<Node> node29 = MakeShared<Node>(1840.975, 2483.356, 0);
	// TSharedPtr<Node> node30 = MakeShared<Node>(1795.610, 2406.520, 0);
	// TSharedPtr<Node> node31 = MakeShared<Node>(1730.026, 2397.067, 0);
	// TSharedPtr<Node> node32 = MakeShared<Node>(1641.279, 2376.692, 0);
	// TSharedPtr<Node> node33 = MakeShared<Node>(1549.777, 2358.181, 0);
	// TSharedPtr<Node> node34 = MakeShared<Node>(1458.542, 2337.467, 0);
	// TSharedPtr<Node> node35 = MakeShared<Node>(1370.094, 2320.976, 0);
	// TSharedPtr<Node> node36 = MakeShared<Node>(1316.307, 2314.063, 0);
	// TSharedPtr<Node> node37 = MakeShared<Node>(1277.770, 2276.628, 0);
	// TSharedPtr<Node> node38 = MakeShared<Node>(1197.056, 2241.291, 0);
	// TSharedPtr<Node> node39 = MakeShared<Node>(1090.378, 2213.529, 0);
	// TSharedPtr<Node> node40 = MakeShared<Node>(978.517, 2181.847, 0);
	// TSharedPtr<Node> node41 = MakeShared<Node>(877.711, 2190.958, 0);
	// TSharedPtr<Node> node42 = MakeShared<Node>(804.365, 2201.632, 0);
	// TSharedPtr<Node> node43 = MakeShared<Node>(717.986, 2183.307, 0);
	// TSharedPtr<Node> node44 = MakeShared<Node>(656.562, 2191.527, 0);
	// TSharedPtr<Node> node45 = MakeShared<Node>(569.210, 2174.382, 0);
	// TSharedPtr<Node> node46 = MakeShared<Node>(482.074, 2154.286, 0);
	// TSharedPtr<Node> node47 = MakeShared<Node>(389.835, 2139.345, 0);
	// TSharedPtr<Node> node48 = MakeShared<Node>(311.907, 2115.287, 0);
	// TSharedPtr<Node> node49 = MakeShared<Node>(244.292, 2092.048, 0);
	// TSharedPtr<Node> node50 = MakeShared<Node>(162.861, 2074.698, 0);
	// TSharedPtr<Node> node51 = MakeShared<Node>(81.431, 2057.349, 0);
	// TSharedPtr<Node> node52 = MakeShared<Node>(0.000, 2040.000, 0);
	// //
	// //
	// add_conn(node1, node2);
	// add_conn(node2, node3);
	// add_conn(node3, node4);
	// add_conn(node4, node5);
	// add_conn(node5, node6);
	// add_conn(node6, node7);
	// add_conn(node7, node8);
	// add_conn(node8, node9);
	// add_conn(node9, node10);
	// add_conn(node10, node11);
	// add_conn(node11, node12);
	// add_conn(node12, node13);
	// add_conn(node13, node14);
	// add_conn(node14, node15);
	// add_conn(node15, node16);
	// add_conn(node16, node17);
	// add_conn(node17, node18);
	// add_conn(node18, node19);
	// add_conn(node19, node20);
	// add_conn(node20, node21);
	// add_conn(node21, node22);
	// add_conn(node22, node23);
	// add_conn(node23, node24);
	// add_conn(node24, node25);
	// add_conn(node25, node26);
	// add_conn(node26, node27);
	// add_conn(node27, node28);
	// add_conn(node28, node29);
	// add_conn(node29, node30);
	// add_conn(node30, node31);
	// add_conn(node31, node32);
	// add_conn(node32, node33);
	// add_conn(node33, node34);
	// add_conn(node34, node35);
	// add_conn(node35, node36);
	// add_conn(node36, node37);
	// add_conn(node37, node38);
	// add_conn(node38, node39);
	// add_conn(node39, node40);
	// add_conn(node40, node41);
	// add_conn(node41, node42);
	// add_conn(node42, node43);
	// add_conn(node43, node44);
	// add_conn(node44, node45);
	// add_conn(node45, node46);
	// add_conn(node46, node47);
	// add_conn(node47, node48);
	// add_conn(node48, node49);
	// add_conn(node49, node50);
	// add_conn(node50, node51);
	// add_conn(node51, node52);
	// add_conn(node52, node1);
	// roads.Add(node1);
	// roads.Add(node2);
	// roads.Add(node3);
	// roads.Add(node4);
	// roads.Add(node5);
	// roads.Add(node6);
	// roads.Add(node7);
	// roads.Add(node8);
	// roads.Add(node9);
	// roads.Add(node10);
	// roads.Add(node11);
	// roads.Add(node12);
	// roads.Add(node13);
	// roads.Add(node14);
	// roads.Add(node15);
	// roads.Add(node16);
	// roads.Add(node17);
	// roads.Add(node18);
	// roads.Add(node19);
	// roads.Add(node20);
	// roads.Add(node21);
	// roads.Add(node22);
	// roads.Add(node23);
	// roads.Add(node24);
	// roads.Add(node25);
	// roads.Add(node26);
	// roads.Add(node27);
	// roads.Add(node28);
	// roads.Add(node29);
	// roads.Add(node30);
	// roads.Add(node31);
	// roads.Add(node32);
	// roads.Add(node33);
	// roads.Add(node34);
	// roads.Add(node35);
	// roads.Add(node36);
	// roads.Add(node37);
	// roads.Add(node38);
	// roads.Add(node39);
	// roads.Add(node40);
	// roads.Add(node41);
	// roads.Add(node42);
	// roads.Add(node43);
	// roads.Add(node44);
	// roads.Add(node45);
	// roads.Add(node46);
	// roads.Add(node47);
	// roads.Add(node48);
	// roads.Add(node49);
	// roads.Add(node50);
	// roads.Add(node51);
	// roads.Add(node52);

	double EndTime7 = FPlatformTime::Seconds();

	double StartTime8 = FPlatformTime::Seconds();
	shrink_roads();
	double EndTime8 = FPlatformTime::Seconds();
	double StartTime9 = FPlatformTime::Seconds();
	get_closed_figures(roads, figures_array, 200);
	double EndTime9 = FPlatformTime::Seconds();
	double StartTime10 = FPlatformTime::Seconds();
	process_blocks(figures_array);
	double EndTime10 = FPlatformTime::Seconds();

	for (int i = 0; i < roads.Num(); i++)
	{
		if (roads[i]->conn.Num() != roads[i]->get_node()->blocks_nearby.Num())
		{
			FVector def_point = roads[i]->get_point();
			debug_points_array_.Add(def_point);
		}
	}

	double StartTime11 = FPlatformTime::Seconds();
	for (auto& b : figures_array)
	{
		process_houses(b);
	}
	double EndTime11 = FPlatformTime::Seconds();
	[[maybe_unused]] double time1 = EndTime1 - StartTime1;
	[[maybe_unused]] double time3 = EndTime3 - StartTime3;
	[[maybe_unused]] double time4 = EndTime4 - StartTime4;
	[[maybe_unused]] double time5 = EndTime5 - StartTime5;
	[[maybe_unused]] double time6 = EndTime6 - StartTime6;
	[[maybe_unused]] double time7 = EndTime7 - StartTime7;
	[[maybe_unused]] double time8 = EndTime8 - StartTime8;
	[[maybe_unused]] double time9 = EndTime9 - StartTime9;
	[[maybe_unused]] double time10 = EndTime10 - StartTime10;
	[[maybe_unused]] double time11 = EndTime11 - StartTime11;
	figures_array_ = figures_array;
	river_figure_ = river_figure;
	map_borders_array_ = map_borders_array;
	roads_ = roads;
}

void TerrainGen::create_guiding_rivers()
{
	FVector start_river_point(0, 0, 0);
	FVector end_river_point(0, y_size, 0);
	auto start_point = MakeShared<Node>((start_river_point + end_river_point) / 2);

	FVector end_river =
		AllGeometry::create_segment_at_angle(FVector(0, 0, 0), FVector{0, y_size, 0}, start_point->get_point(),
											 -90 + (rand() % 20), (rand() % 20 + 10) * (av_river_length));

	auto end_point = MakeShared<Node>(end_river);
	auto start_point_left = MakeShared<Node>(FVector(0, y_size / 2 - av_river_length / 2, 0));
	auto start_point_right = MakeShared<Node>(FVector(0, y_size / 2 + av_river_length / 2, 0));
	river.Add(start_point_left);
	river.Add(start_point_right);
	// guiding_river.Add(start_point);
	add_conn(start_point_left, start_point_right);
	create_guiding_river_segment(start_point, end_point, start_point_left, start_point_right);
}

void TerrainGen::create_guiding_river_segment(TSharedPtr<Node> start_point, TSharedPtr<Node> end_point,
											  TSharedPtr<Node> start_point_left, TSharedPtr<Node> start_point_right)
{
	guiding_river.AddUnique(start_point);
	bool is_ending = false;

	auto intersect_river = AllGeometry::is_intersect_array(start_point, end_point, guiding_river, false);
	auto intersect_border = AllGeometry::is_intersect_array(start_point, end_point, map_borders_array, false);
	if (intersect_border.IsSet())
	{
		is_ending = true;
		end_point->set_point(intersect_border->Key);
	}
	else if (intersect_river.IsSet())
	{
		is_ending = true;
		end_point->set_point(intersect_river->Key);
	}
	TSharedPtr<Node> end_point_left = MakeShared<Node>(AllGeometry::create_segment_at_angle(
		start_point->get_point(), end_point->get_point(), end_point->get_point(), -90, av_river_length / 2));
	TSharedPtr<Node> end_point_right = MakeShared<Node>(AllGeometry::create_segment_at_angle(
		start_point->get_point(), end_point->get_point(), end_point->get_point(), 90, av_river_length / 2));
	river.AddUnique(end_point_left);
	river.AddUnique(end_point_right);

	if (intersect_border.IsSet())
	{
		end_point_left->set_used();
		end_point_right->set_used();
	}

	TSharedPtr<Node> old_node = start_point;
	TSharedPtr<Node> old_node_left = start_point_left;
	TSharedPtr<Node> old_node_right = start_point_right;
	add_conn(start_point, old_node);
	int dist_times = FVector::Distance(start_point->get_point(), end_point->get_point()) / (av_river_length);
	for (int i = 1; i <= dist_times; i++)
	{
		bridges.Add(MakeTuple(old_node_left, old_node_right));
		auto node_ptr = MakeShared<Node>();
		TSharedPtr<Node> node_ptr_left =
			MakeShared<Node>(start_point_left->get_point() +
							 ((end_point_left->get_point() - start_point_left->get_point()) / dist_times * i));
		TSharedPtr<Node> node_ptr_right =
			MakeShared<Node>(start_point_right->get_point() +
							 ((end_point_right->get_point() - start_point_right->get_point()) / dist_times * i));
		node_ptr_left->set_type(point_type::river);
		node_ptr_right->set_type(point_type::river);
		if (i != dist_times)
		{
			create_segment(river, node_ptr_left, old_node_left, true, point_type::river, max_river_length);
			create_segment(river, node_ptr_right, old_node_right, true, point_type::river, max_river_length);
		}
		else
		{
			node_ptr_left = end_point_left;
			node_ptr_right = end_point_right;
			create_segment(river, end_point_left, old_node_left, true, point_type::river, max_river_length);
			create_segment(river, end_point_right, old_node_right, true, point_type::river, max_river_length);
			// end_point->add_connection(old_node, false);
			// old_node->add_connection(end_point, false);
		}
		river.AddUnique(node_ptr_left);
		river.AddUnique(node_ptr_right);
		// create_segment(river, node_ptr_left, node_ptr_right, true, point_type::river, max_river_length);
		old_node_left = node_ptr_left;
		old_node_right = node_ptr_right;
	}

	if (!is_ending)
	{
		Node next_segment = Node(AllGeometry::create_segment_at_angle(start_point->get_point(), end_point->get_point(),
																	  end_point->get_point(), -60 + (rand() % 120),
																	  (rand() % 20 + 10) * (av_river_length)));
		create_guiding_river_segment(end_point, MakeShared<Node>(next_segment), old_node_left, old_node_right);
		if (rand() % 4 >= 3)
		{
			auto next_segment1 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_point(), end_point->get_point(), end_point->get_point(), -60 + (rand() % 120),
				(rand() % 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment1, old_node_left, old_node_right);
		}
		if (rand() % 8 >= 7)
		{
			auto next_segment2 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_point(), end_point->get_point(), end_point->get_point(), -60 + (rand() % 120),
				(rand() % 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment2, old_node_left, old_node_right);
		}
	}
	else
	{
		add_conn(old_node_left, old_node_right);
	}
}
void TerrainGen::process_bridges()
{
	bridges.RemoveAll(
		[&](const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& A)
		{
			return rand() % 5 >= 4 ||
				FVector::Distance(A.Key->get_point(), A.Value->get_point()) > (max_river_length * 3 / 2) ||
				AllGeometry::is_intersect_array_count(A.Key, A.Value, river, true) % 2 != 0;
		});

	Algo::Sort(
		bridges,
		[](const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& A, const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& B)
		{
			return FVector::Distance(A.Key->get_point(), A.Value->get_point()) <
				FVector::Distance(B.Key->get_point(), B.Value->get_point());
		});
}


void TerrainGen::create_guiding_roads()
{
	// double av_size = (x_size+y_size)/2;
	constexpr double point_row_counter = 9;
	for (double x = 1; x < point_row_counter; x++)
	{
		for (double y = 1; y < point_row_counter; y++)
		{
			auto x_val = x_size / point_row_counter * x +
				(rand() % (static_cast<int>(x_size / point_row_counter / 2))) - (x_size / point_row_counter / 4);
			auto y_val = y_size / point_row_counter * y +
				(rand() % (static_cast<int>(x_size / point_row_counter / 2))) - (x_size / point_row_counter / 4);
			roads.Add(MakeShared<Node>(Node{FVector(x_val, y_val, 0)}));
		}
	}
	for (auto& r : roads)
	{
		for (int iter = 0; iter < 10; iter++)
		{
			point_shift(r->get_node()->point);
		}
	}
	TArray<TTuple<double, TSharedPtr<Node>>> weighted_road_centers;
	for (auto& r : roads)
	{
		if (FVector::Distance(r->get_point(), FVector{x_size / 2, y_size / 2, 0}) < (y_size / 3))
		{
			bool is_break = false;
			for (auto riv : river)
			{
				if (FVector::Distance(r->get_point(), riv->get_point()) < river_road_distance)
				{
					is_break = true;
					break;
				}
			}
			if (is_break)
			{
				continue;
			}
			double weight = 0;
			for (auto& w : weighted_points)
			{
				if (FVector::Distance(r->get_point(), w.point) < w.weight)
				{
					weight += (w.weight - FVector::Distance(r->get_point(), w.point));
				}
			}
			weighted_road_centers.Add(TTuple<double, TSharedPtr<Node>>{weight, r});
		}
	}

	Algo::Sort(weighted_road_centers,
			   [](const TTuple<double, TSharedPtr<Node>>& A, const TTuple<double, TSharedPtr<Node>>& B)
			   { return A.Get<0>() < B.Get<0>(); });
	roads.Reset();
	// road.Add(weighted_road_centers[0].Value);
	for (int i = 1; i < weighted_road_centers.Num(); i++)
	{
		bool is_break = false;
		for (auto r : roads)
		{
			if (FVector::Distance(weighted_road_centers[i].Value->get_point(), r->get_point()) < (y_size / 7))
			{
				is_break = true;
				break;
			}
		}
		if (!is_break)
		{
			roads.Add(weighted_road_centers[i].Value);
		}
	}
	// placing bridges
	int bridges_num = 15;
	if (bridges_num > bridges.Num())
	{
		bridges_num = bridges.Num();
	}
	for (int i = 0; i < bridges_num; i++)
	{
		auto bridge1 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
			bridges[i].Key->get_point(), bridges[i].Value->get_point(), bridges[i].Value->get_point(), 0, 20));
		auto bridge2 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
			bridges[i].Value->get_point(), bridges[i].Key->get_point(), bridges[i].Key->get_point(), 0, 20));
		add_conn(bridge1, bridge2);
		roads.Add(bridge1);
		roads.Add(bridge2);
	}
	// for (auto r : river)
	// {
	// 	if (FVector::Distance(r->get_point(), center) < (y_size / 3) && rand() % 8 >= 2 && r->conn.Num() == 2)
	// 	{
	// 		bool too_near = false;
	// 		for (auto road : roads)
	// 		{
	// 			if (FVector::Distance(road->get_point(), r->get_point()) < y_size / 15)
	// 			{
	// 				too_near = true;
	// 				break;
	// 			}
	// 		}
	// 		if (!too_near)
	// 		{
	// 			auto bridge1 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
	// 				r->conn[0]->node->get_point(), r->get_point(), r->get_point(), 90, 20));
	// 			auto bridge2 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
	// 				r->conn[0]->node->get_point(), r->get_point(), r->get_point(), -90, 20));
	// 			add_conn(bridge1, bridge2);
	// 			roads.Add(bridge1);
	// 			roads.Add(bridge2);
	// 		}
	// 	}
	// }
	for (auto r : roads)
	{
		weighted_points.Add(WeightedPoint{r->get_point(), -y_size / 7});
	}

	for (auto& r : roads)
	{
		road_centers.Add(r);
	}

	for (auto& r : road_centers)
	{
		r->set_type(point_type::main_road);
	}

	for (int i = 0; i < road_centers.Num() - 1; i++)
	{
		auto local_road = road_centers;

		local_road.Sort(
			[i, this](TSharedPtr<Node> Item1, TSharedPtr<Node> Item2)
			{
				return FVector::Distance(road_centers[i]->get_point(), Item1->get_point()) <
					FVector::Distance(road_centers[i]->get_point(), Item2->get_point());
			});
		int success_roads = 0;
		for (int j = 0; j < local_road.Num() - 1 && success_roads < 4; j++)
		{
			success_roads += create_guiding_road_segment(road_centers[i], local_road[j]);
		}
	}
}

bool TerrainGen::create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point)
{
	if (AllGeometry::is_intersect_array(start_point, end_point, river, true).IsSet())
	{
		return false;
	}

	TSharedPtr<Node> old_node = start_point;
	// river.Add(old_node);
	int dist_times = FVector::Distance(start_point->get_point(), end_point->get_point()) / (av_road_length);
	for (int i = 1; i <= dist_times; i++)
	{
		TSharedPtr<Node> node = MakeShared<Node>(0, 0, 0);
		node->set_type(point_type::main_road);
		node->set_point(start_point->get_point() +
						((end_point->get_point() - start_point->get_point()) / dist_times * i));
		if (i == dist_times)
		{
			node = end_point;
		}
		create_segment(roads, old_node, node, true, point_type::main_road, max_road_length);
		// add_conn(node, old_node);
		roads.AddUnique(node);
		old_node = node;
	}
	return true;
}

void TerrainGen::shrink_roads()
{
	int road_points = roads.Num();
	int old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = roads.Num();
		auto array = roads;
		roads.RemoveAll(
			[&](TSharedPtr<Node> node)
			{
				TSharedPtr<Node> target_node = nullptr;
				for (auto conn : node->conn)
				{
					if (FVector::Dist(node->get_point(), conn->node->get_point()) < min_road_length)
					{
						target_node = conn->node;
						break;
					}
				}
				if (target_node.IsValid())
				{
					for (auto del_node : node->conn)
					{
						add_conn(del_node->node, target_node);
					}
					node->delete_me();
					return true;
				}
				return false;
			});
		road_points = roads.Num();
	}
	road_points = roads.Num();
	old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = roads.Num();
		roads.RemoveAll(
			[&](TSharedPtr<Node> node)
			{
				if (node->conn.Num() < 2)
				{
					node->delete_me();
					return true;
				}
				return false;
			});
		road_points = roads.Num();
	}
}

void TerrainGen::create_usual_roads()
{
	roads.Sort([this](auto Item1, auto Item2) { return FMath::FRand() < 0.5f; });
	TArray<TSharedPtr<Node>> add_road = roads;
	int forward;
	int left;
	int right;

	for (auto road_node : roads)
	{
		if (!road_node->is_used())
		{
			forward = 2;
			left = 6;
			right = 10;
			if (road_node->conn.Num() == 1)
			{
				if (rand() % 16 >= forward)
				{
					auto length = FVector::Distance(road_node->get_point(), road_node->conn[0]->node->get_point()) +
						(rand() % 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = (rand() % 10) - 5;
					auto line1 = AllGeometry::create_segment_at_angle(road_node->conn[0]->node->get_point(),
																	  road_node->get_point(), road_node->get_point(),
																	  angle_in_degrees, length);

					TSharedPtr<Node> new_node = MakeShared<Node>(line1);
					new_node->set_type(point_type::road);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->get_point(), line1) < rc->conn.Num() * (y_size / 28))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node);
						create_segment(add_road, road_node, new_node, false, point_type::road, max_road_length);
					}
				}
			}
			if (road_node->conn.Num() == 2 || road_node->conn.Num() == 1)
			{
				if (rand() % 16 >= left)
				{
					auto length = FVector::Distance(road_node->get_point(), road_node->conn[0]->node->get_point()) +
						(rand() % 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = 90 + (rand() % 10) - 5;
					auto line2 = AllGeometry::create_segment_at_angle(road_node->conn[0]->node->get_point(),
																	  road_node->get_point(), road_node->get_point(),
																	  angle_in_degrees, length);
					TSharedPtr<Node> new_node2 = MakeShared<Node>(line2);
					new_node2->set_type(point_type::road);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->get_point(), line2) < rc->conn.Num() * (y_size / 28))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node2);
						create_segment(add_road, road_node, new_node2, false, point_type::road, max_road_length);
					}
				}
				if (rand() % 16 >= right)
				{
					auto length = FVector::Distance(road_node->get_point(), road_node->conn[0]->node->get_point()) +
						(rand() % 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = -90 + (rand() % 10) - 5;
					auto line3 = AllGeometry::create_segment_at_angle(road_node->conn[0]->node->get_point(),
																	  road_node->get_point(), road_node->get_point(),
																	  angle_in_degrees, length);
					TSharedPtr<Node> new_node3 = MakeShared<Node>(line3);
					new_node3->set_type(point_type::road);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->get_point(), line3) < rc->conn.Num() * (y_size / 28))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node3);
						create_segment(add_road, road_node, new_node3, false, point_type::road, max_road_length);
					}
				}
			}
		}

		road_node->set_used();
	}
	for (auto a : add_road)
	{
		bool is_in_road = false;
		for (auto r : roads)
		{
			if (a->get_point() == r->get_point())
			{
				is_in_road = true;
				// a = r;
				break;
			}
		}
		if (!is_in_road)
		{
			roads.AddUnique(a);
		}
	}
	// roads += add_road;

	for (int i = 0; i < roads.Num(); i++)
	{
		if (!roads[i]->is_used())
		{
			for (int count = 0; count < 3; count++)
			{
				move_road(roads[i]);
			};
		}
	}
}

TOptional<TSharedPtr<Node>> TerrainGen::create_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
													   TSharedPtr<Node> end_point, bool to_exect_point, point_type type,
													   double max_length)
{
	TSharedPtr<Node> backup_endpoint = end_point;
	do
	{
		end_point = backup_endpoint;
		bool force_end = false;
		if ((end_point->get_point().X) < 0)
		{
			end_point->set_point_X(0);
			force_end = true;
		}
		if ((end_point->get_point().Y) < 0)
		{
			end_point->set_point_Y(0);
			force_end = true;
		}
		if ((end_point->get_point().X) > x_size)
		{
			end_point->set_point_X(x_size);
			force_end = true;
		}
		if ((end_point->get_point().Y) > y_size)
		{
			end_point->set_point_Y(y_size);
			force_end = true;
		}
		end_point->set_type(type);
		if (force_end)
		{
			return end_point;
		}
		int tries = 0;
		auto intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, array, false);
		while (intersection.IsSet())
		{
			end_point = intersection.GetValue();
			tries++;
			if (tries > 10 || FVector::Distance(start_point->get_point(), end_point->get_point()) > max_length
				// ||				to_exect_point
			)
			{
				// auto presise_intersect = AllGeometry::is_intersect_array(start_point->get_point(),
				// backup_endpoint->get_point(), array, false);
				// if (presise_intersect.IsSet())
				// {
				// 	end_point =
				// insert_conn(presise_intersect->Value.Key,
				// presise_intersect->Value.Value,
				// 							presise_intersect->Key);
				// }
				break;
			}
			intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, array, false);
		}
		if (tries > 10 || FVector::Distance(start_point->get_point(), end_point->get_point()) > max_length ||
			to_exect_point)
		{
			auto presise_intersect =
				AllGeometry::is_intersect_array(start_point->get_point(), backup_endpoint->get_point(), array, false);
			if (presise_intersect.IsSet())
			{
				end_point =
					insert_conn(presise_intersect->Value.Key, presise_intersect->Value.Value, presise_intersect->Key);
			}
		}
		TOptional<FVector> river_intersection;
		if (type != point_type::river)
		{
			river_intersection =
				AllGeometry::is_intersect_array_clear(start_point->get_point(), end_point->get_point(), river, true);
		}
		if (!river_intersection.IsSet())
		{
			add_conn(start_point, end_point);
			array.AddUnique(end_point);
		}
		else
		{
			return TOptional<TSharedPtr<Node>>();
		}
		if (!to_exect_point)
		{
			return end_point;
		}
		start_point = end_point;
	}
	while (end_point->get_point() != backup_endpoint->get_point());

	if (end_point->get_point() == backup_endpoint->get_point())
	{
		end_point = backup_endpoint;
	}
	return end_point;
}

void TerrainGen::point_shift(FVector& point)
{
	FVector bias(0, 0, 0);
	double biggest_weight = 0;
	for (int j = 0; j < weighted_points.Num(); j++)
	{
		double length = FVector::Distance(point, weighted_points[j].point);
		if (length < abs(weighted_points[j].weight) && biggest_weight < weighted_points[j].weight - length)
		{
			biggest_weight = weighted_points[j].weight - length;
			bias = ((point - weighted_points[j].point) / length) * (weighted_points[j].weight - length) / 50;
		}
	}
	point += bias;
}

void TerrainGen::get_closed_figures(TArray<TSharedPtr<Node>> lines, TArray<District>& fig_array, int figure_threshold)
{
	for (auto l : lines)
	{
		for (auto lconn : l->conn)
		{
			if (!lconn->figure->IsEmpty() || lconn->not_in_figure)
			{
				// UE_LOG(LogTemp, Warning, TEXT("Продолжили!"));
				continue;
			}
			// UE_LOG(LogTemp, Warning, TEXT("----------Начал вывод фигуры, размер фигуры %d"), lines.Num());
			TSharedPtr<TArray<TSharedPtr<Point>>> figure_array = MakeShared<TArray<TSharedPtr<Point>>>();
			TArray<TSharedPtr<Conn>> conn_array;
			conn_array.Add(lconn);
			auto first_node = l;

			// first_node->print_connections();
			auto second_node = lconn->node;
			// second_node->print_connections();
			figure_array->Add(l->get_node());
			figure_array->Add(lconn->node->get_node());
			TSharedPtr<Node> rightest_node;
			TSharedPtr<Conn> this_conn;
			bool some_error = false;
			bool not_in_figure = false;
			double general_angle = 0;
			// while (!figure_array->Contains(second_node))
			while (second_node->get_point() != l->get_node()->point)
			{
				double smallest_angle = 360;
				for (int i = 0; i < second_node->conn.Num(); i++)
				{
					double angle =
						AllGeometry::calculate_angle(second_node->conn[i]->node->get_point(), second_node->get_point(),
													 first_node->get_point(), true);

					if (angle < smallest_angle && angle > 1)
					{
						smallest_angle = angle;
						rightest_node = second_node->conn[i]->node;
						this_conn = second_node->conn[i];
					}
				}
				general_angle += smallest_angle;
				if (smallest_angle == 360)
				{
					not_in_figure = true;
					break;
				}

				// UE_LOG(LogTemp, Warning, TEXT("Добавили точку %f, %f, index %d"), rightest_node->get_node()->point.X,
				// 	   rightest_node->get_node()->point.Y, rightest_node->debug_ind_);
				// rightest_node->print_connections();
				if (!this_conn->figure->IsEmpty())
				{
					some_error = true;
					break;
				}
				figure_array->Add(rightest_node->get_node());
				conn_array.Add(this_conn);

				if (figure_array->Num() > figure_threshold)
				{
					some_error = true;
					break;
				}
				first_node = second_node;
				second_node = rightest_node;
			}
			if (some_error)
			{
				continue;
			}
			else if (not_in_figure)
			{
				for (auto conn : conn_array)
				{
					conn->not_in_figure = true;
				}
			}
			else
			{
				for (auto conn : conn_array)
				{
					conn->figure = figure_array;
				}
				if (general_angle / (figure_array->Num() - 1) < 180)
				{
					// UE_LOG(LogTemp, Warning, TEXT("Фигура добавлена, размер %d"), figure_array->Num());
					fig_array.Add(District(*figure_array));
				}
			}
		}
	}
}
void TerrainGen::get_river_figure()
{
	TArray<District> river_fig_array;
	get_closed_figures(river, river_fig_array, 1000);
	if (river_fig_array.IsEmpty())
	{
		return;
	}
	river_fig_array.Sort([this](District Item1, District Item2) { return Item1.figure.Num() > Item2.figure.Num(); });

	river_figure = river_fig_array[0];
	river.RemoveAll(
		[&](TSharedPtr<Node> A)
		{
			for (auto exist_f : river_figure.figure)
			{
				if (A->get_point() == exist_f->point)
				{
					return false;
				}
			}
			A->delete_me();
			return true;
		});

	river.RemoveAll(
		[&](TSharedPtr<Node>& r)
		{
			for (int i = 1; i < river_figure.figure.Num() - 1; i++)
			{
				if (r->conn.Num() > 2)
				{
					TSharedPtr<Node> prev;
					TSharedPtr<Node> next;
					for (auto c : r->conn)
					{
						if (c->node->get_point() == river_figure.figure[i - 1]->point)
						{
							prev = c->node;
						}
						if (c->node->get_point() == river_figure.figure[i + 1]->point)
						{
							next = c->node;
						}
					}
					if (prev && next)
					{
						add_conn(prev, next);
						r->delete_me();
						return true;
					}
				}
			}
			return false;
		});

	bridges.RemoveAll(
		[&](const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& A)
		{
			bool is_key_in = false;
			bool is_value_in = false;
			for (auto river_point : river)
			{
				if (A.Key->get_point() == river_point->get_point())
				{
					is_key_in = true;
					continue;
				}
				if (A.Value->get_point() == river_point->get_point())
				{
					is_value_in = true;
					continue;
				}
			}
			return is_key_in && is_value_in ? false : true;
		});
}
// void TerrainGen::smooth_blocks(TArray<Block>& blocks)
// {
// 	for (auto& b : blocks)
// 	{
// 		int fig_num = b.figure.Num();
// 		for (int i = 1; i <= fig_num; i++)
// 		{
// 			if (AllGeometry::calculate_angle(b.figure[i - 1]->point, b.figure[i % fig_num]->point,
// 											 b.figure[(i + 1) % fig_num]->point, true) > 180)
// 			{
// 				b.figure[i % fig_num]->point = (b.figure[i - 1]->point + b.figure[(i + 1) % fig_num]->point) / 2;
// 			}
// 		}
// 		b.area = AllGeometry::get_poygon_area(b.figure);
// 	}
// }
void TerrainGen::process_blocks(TArray<District>& blocks)
{
	// blocks.RemoveAll([this](District& Item1) { return !(Item1.shrink_size(Item1.self_figure, 0.0f)); });
	int wrong_blocks = 0;
	// for (auto& b : blocks)
	// {
	// 	b.get_self_figure();
	// 	if (!b.shrink_size(b.self_figure, 0.5f))
	// 	{
	// 		wrong_blocks++;
	// 	}
	// 	b.area = AllGeometry::get_poygon_area(b.self_figure);
	// }


	blocks.Sort([this](District Item1, District Item2) { return Item1.area > Item2.area; });
	double royal_area = 0;
	bool royal_found = false;
	// bool dock_found = false;
	for (auto& b : blocks)
	{
		bool is_river = false;
		for (auto r : river)
		{
			if (b.is_point_in_self_figure(r->get_point()))
			{
				is_river = true;
				b.set_type(block_type::dock);
				break;
			}
		}
		if (b.get_type() == block_type::unknown && !royal_found && royal_area == 0)
		{
			bool point1 = false;
			int is_in_main = 0;
			for (auto& p : b.figure)
			{
				if (is_river)
				{
					break;
				}
				if (FVector::Distance(p->point, center) < (x_size + y_size) / 10)
				{
					point1 = true;
				}
				if (p->type == point_type::main_road)
				{
					is_in_main++;
				}
				if (point1 && is_in_main >= 3)
				{
					b.set_type(block_type::royal);
					royal_area += b.area;
					break;
				}
			}
		}
		if (b.get_type() == block_type::unknown && royal_area > 0 && !royal_found)
		{
			for (auto p : b.figure)
			{
				for (auto block_near : p->blocks_nearby)
				{
					if (block_near == block_type::royal)
					{
						b.set_type(block_type::royal);
						royal_area += b.area;
						break;
					}
				}
				if (b.get_type() == block_type::royal)
				{
					break;
				}
			}
		}
		if (b.get_type() == block_type::royal && royal_area > 1000)
		{
			royal_found = true;
		}
		if (b.get_type() == block_type::unknown)
		{
			bool is_near_royal = false;
			bool is_near_dock = false;
			// bool is_near_slums = false;
			// bool is_near_residential = false;
			for (auto p : b.figure)
			{
				if (p->blocks_nearby.Contains(block_type::royal))
				{
					is_near_royal = true;
				}
				if (p->blocks_nearby.Contains(block_type::dock))
				{
					is_near_dock = true;
				}

				// if (p->blocks_nearby.Contains(block_type::slums))
				// {
				// 	is_near_slums = true;
				// }
				// if (p->blocks_nearby.Contains(block_type::residential))
				// {
				// 	is_near_residential = true;
				// }
			}
			if (is_near_royal && !is_near_dock && b.area > 6000)
			{
				b.set_type(block_type::luxury);
			}
		}
	}
	int blocks_count = blocks.Num();
	int named_blocks = 0;
	int old_named_blocks;
	do
	{
		old_named_blocks = named_blocks;
		named_blocks = 0;
		for (auto& b : blocks)
		{
			if (b.get_type() != block_type::unknown)
			{
				named_blocks++;
			}
			else
			{
				TMap<block_type, int32> ElementCount;
				for (auto& fig : b.figure)
				{
					for (auto block : fig->blocks_nearby)
					{
						ElementCount.FindOrAdd(block)++;
					}
				}
				int blocks_near = 0;
				for (auto el : ElementCount)
				{
					blocks_near += el.Value;
				}
				if (ElementCount.IsEmpty())
				{
					continue;
				}
				int royal_count = 0;
				int dock_count = 0;
				int luxury_count = 0;
				int residential_count = 0;
				int slums_count = 0;
				for (auto el : ElementCount)
				{
					switch (el.Key)
					{
					case block_type::dock:
						{
							dock_count = el.Value;
							break;
						}
					case block_type::royal:
						{
							royal_count = el.Value;
							break;
						}
					case block_type::luxury:
						{
							luxury_count = el.Value;
							break;
						}
					case block_type::residential:
						{
							residential_count = el.Value;
							break;
						}
					case block_type::slums:
						{
							slums_count = el.Value;
							break;
						}
					default:
						break;
					}
				}
				if (blocks_near == 0)
				{
					continue;
				}
				int koeff = (dock_count * (-4) + royal_count * 6 + luxury_count * 3 + slums_count * (-8) +
							 residential_count * 2) /
					blocks_near;

				if (koeff <= -7 && luxury_count == 0 && royal_count == 0)
				{
					b.set_type(block_type::slums);
				}
				else if (koeff >= 0 && koeff < 4)
				{
					b.set_type(block_type::residential);
				}
				else if (koeff >= 4 && slums_count != 0 && dock_count != 0)
				{
					b.set_type(block_type::luxury);
				}
				else
				{
					int rand_val = rand() % 100;
					if (rand_val > 85 && slums_count != 0 && dock_count != 0)
					{
						b.set_type(block_type::luxury);
					}
					else if (rand_val > 50)
					{
						b.set_type(block_type::residential);
					}
					else if (luxury_count == 0 && royal_count == 0)
					{
						b.set_type(block_type::slums);
					}
				}
			}
		}
	}
	while (named_blocks < blocks_count && old_named_blocks != named_blocks);
}
void TerrainGen::process_houses(District& block)
{
	FVector block_center(0, 0, 0);
	for (auto p : block.figure)
	{
		block_center += p->point;
	}
	block_center /= block.self_figure.Num();
	if (block.get_type() == block_type::luxury)
	{
		if (!block.is_point_in_self_figure(block_center))
		{
			return;
		}
		for (int i = 1; i < block.self_figure.Num(); i++)
		{
			if (block.self_figure[i - 1].blocks_nearby.Contains(block_type::royal) &&
				block.self_figure[i].blocks_nearby.Contains(block_type::royal))
			{
				FVector point1 = AllGeometry::create_segment_at_angle(block.self_figure[i - 1].point,
																	  block.self_figure[i].point, block_center, 0, 40);
				FVector point2 = AllGeometry::create_segment_at_angle(
					block.self_figure[i - 1].point, block.self_figure[i].point, block_center, 180, 40);
				TArray<FVector> figure{point1, point2};
				if (block.create_house(figure, 40, 30))
				{
					break;
				}
			}
		}
	}
	else if (block.get_type() == block_type::residential)
	{
		int self_figure_count = block.self_figure.Num();
		for (int i = 1; i <= self_figure_count; i++)
		{
			FVector point1 = block.self_figure[i - 1].point;
			FVector point2 = block.self_figure[i % self_figure_count].point;
			double dist = FVector::Distance(point1, point2);
			double general_width = 0;
			for (; general_width < dist;)
			{
				double width = rand() % 3 + 5;
				if (rand() % 7 > 1)
				{
					double length = rand() % 5 + 10;
					double height = (rand() % 2 + 1) * 4;
					FVector point_beg = AllGeometry::create_segment_at_angle(
						point1, point2, (point2 - point1) / dist * (general_width + (width / 2)) + point1, 90, 1);
					FVector point_end = AllGeometry::create_segment_at_angle(point1, point2, point_beg, 90, length);
					TArray<FVector> figure{point_beg, point_end};
					block.create_house(figure, width, height);
				}
				general_width += (width + 1);
			}
		}
	}
}
