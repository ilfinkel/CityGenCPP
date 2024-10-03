#include "MainTerrain.h"

// #include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() { PrimaryActorTick.bCanEverTick = true; }

void AMainTerrain::BeginPlay()
{
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();


	create_terrain();
	get_closed_figures(roads);
	process_blocks(figures_array);
	draw_all();
}

// Called every frame
void AMainTerrain::Tick(float DeltaTime)
{
	// Super::Tick(DeltaTime);
	//// create_usual_roads();
	// draw_all();
}
void AMainTerrain::add_conn(TSharedPtr<Node> node1, TSharedPtr<Node> node2)
{
	if (!node1->get_next_point(node2->get_node()).IsSet())
	{
		node1->add_connection(node2);
	}
	if (!node2->get_next_point(node1->get_node()).IsSet())
	{
		node2->add_connection(node1);
	}
}
TSharedPtr<Node> AMainTerrain::insert_conn(TSharedPtr<Node> node1_to_insert, TSharedPtr<Node> node2_to_insert,
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

void AMainTerrain::tick_river(TSharedPtr<Node>& node)
{
	auto backup_node = node;

	point_shift(node->get_node()->point);
	auto intersec = AllGeometry::is_intersect_array(node, backup_node, map_borders_array, true);
	if (intersec.IsSet())
	{
		node->set_point(intersec->Key);
	}

	FVector all_point(0, 0, 0);
	double count = 0;
	for (int i = 0; i < node->conn.Num(); i++)
	{
		all_point += node->conn[i]->node->get_point();
		count++;
	}
	bool is_break = false;
	for (int i = 0; i < node->conn.Num(); i++)
	{
		if (is_break)
		{
			break;
		}

		if (FVector::Distance(node->get_point(), node->conn[i]->node->get_point()) > (y_size / 15))
		{
			node->get_point() = all_point / count;
			break;
		}
	}
}

void AMainTerrain::tick_road(TSharedPtr<Node>& node)
{
	if (node->is_used())
	{
		return;
	}
	auto backup_node = node;
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
	}
}

void AMainTerrain::create_terrain()
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
	map_borders_array.Add(map_node1);
	map_borders_array.Add(map_node2);
	map_borders_array.Add(map_node3);
	map_borders_array.Add(map_node4);
	double points_count = 64;
	double av_size = (x_size + y_size) / 2;

	weighted_points.Empty();

	const double point_row_counter = sqrt(points_count);
	for (double x = 1; x < point_row_counter; x++)
	{
		for (int y = 1; y < point_row_counter; y++)
		{
			weighted_points.Add(WeightedPoint{
				FVector(static_cast<int>(x_size / point_row_counter * x +
										 (rand() % (static_cast<int>(x_size / point_row_counter / 2))) -
										 (x_size / point_row_counter / 4)),
						static_cast<int>(y_size / point_row_counter * y) +
							(rand() % (static_cast<int>(x_size / point_row_counter / 2))) -
							(x_size / point_row_counter / 4),
						0),
				(rand() % static_cast<int>(av_size / point_row_counter / 2)) + (av_size / point_row_counter / 3)});
		}
	}

	create_guiding_rivers();
	for (int iter = 0; iter < 100; iter++)
	{
		for (auto& r : river)
		{
			tick_river(r);
		}
	}

	create_guiding_roads();

	for (int iter = 0; iter < 100; iter++)
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
			tick_road(r);
		}
	}

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
			tick_road(r);
		}
	}
	shrink_roads();

	// auto test_node_node1 = MakeShared<Node>(300, 300, 0);
	// auto test_node_node2 = MakeShared<Node>(300, y_size - 300, 0);
	// auto test_node_node3 = MakeShared<Node>(x_size - 300, y_size - 300, 0);
	// auto test_node_node4 = MakeShared<Node>(x_size - 300, 300, 0);
	// auto test_node_node5 = MakeShared<Node>(x_size, 300, 0);
	//
	// roads.Add(test_node_node1);
	// roads.Add(test_node_node2);
	// roads.Add(test_node_node3);
	// roads.Add(test_node_node4);
	// roads.Add(test_node_node5);
	// add_conn(test_node_node1, test_node_node2, false);
	// add_conn(test_node_node2, test_node_node3, false);
	// // add_conn(test_node_node3, test_node_node4, false);
	// add_conn(test_node_node4, test_node_node1, false);
	// add_conn(test_node_node5, test_node_node3, false);
	// add_conn(test_node_node5, test_node_node4, false);
	// add_conn(test_node_node3, test_node_node1, true);
}

void AMainTerrain::create_guiding_rivers()
{
	FVector start_river_point(0, 0, 0);
	FVector end_river_point(0, y_size, 0);
	auto start_point = MakeShared<Node>((start_river_point + end_river_point) / 2);

	FVector end_river =
		AllGeometry::create_segment_at_angle(FVector(0, 0, 0), FVector{0, y_size, 0}, start_point->get_point(),
											 -90 + (rand() % 20), (rand() % 20 + 10) * (av_river_length));
	auto end_point = MakeShared<Node>(end_river);
	create_guiding_river_segment(start_point, end_point);
}

void AMainTerrain::create_guiding_river_segment(TSharedPtr<Node> start_point, TSharedPtr<Node> end_point)
{
	river.AddUnique(start_point);
	bool is_ending = false;

	auto intersect_river = AllGeometry::is_intersect_array_clear(start_point, end_point, river, false);
	auto intersect_border = AllGeometry::is_intersect_array(start_point, end_point, map_borders_array, false);
	if (intersect_border.IsSet())
	{
		is_ending = true;
		end_point->set_point(intersect_border->Key);
	}
	else if (intersect_river.IsSet())
	{
		is_ending = true;
		end_point = intersect_river.GetValue();
	}

	TSharedPtr<Node> old_node = start_point;

	int dist_times = FVector::Distance(start_point->get_point(), end_point->get_point()) / (av_river_length);
	for (int i = 1; i <= dist_times; i++)
	{
		auto node_ptr = MakeShared<Node>();
		if (i != dist_times)
		{
			node_ptr->set_point(start_point->get_point() +
								((end_point->get_point() - start_point->get_point()) / dist_times * i));
			node_ptr->set_type(point_type::river);
			add_conn(node_ptr, old_node);
		}
		else
		{
			add_conn(end_point, old_node);
			// end_point->add_connection(old_node, false);
			// old_node->add_connection(end_point, false);
		}
		river.AddUnique(node_ptr);
		old_node = node_ptr;
	}
	if (!is_ending)
	{
		Node next_segment = Node(AllGeometry::create_segment_at_angle(start_point->get_point(), end_point->get_point(),
																	  end_point->get_point(), -60 + (rand() % 120),
																	  (rand() % 20 + 10) * (av_river_length)));
		create_guiding_river_segment(end_point, MakeShared<Node>(next_segment));
		if (rand() % 4 >= 3)
		{
			auto next_segment1 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_point(), end_point->get_point(), end_point->get_point(), -60 + (rand() % 120),
				(rand() % 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment1);
		}
		if (rand() % 8 >= 3)
		{
			auto next_segment2 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_point(), end_point->get_point(), end_point->get_point(), -60 + (rand() % 120),
				(rand() % 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment2);
		}
	}
}

void AMainTerrain::create_guiding_roads()
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
		for (int iter = 0; iter < 1000; iter++)
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
	for (auto r : river)
	{
		if (FVector::Distance(r->get_point(), center) < (y_size / 3) && rand() % 8 >= 2 && r->conn.Num() == 2)
		{
			bool too_near = false;
			for (auto road : roads)
			{
				if (FVector::Distance(road->get_point(), r->get_point()) < y_size / 15)
				{
					too_near = true;
					break;
				}
			}
			if (!too_near)
			{
				auto bridge1 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
					r->conn[0]->node->get_point(), r->get_point(), r->get_point(), 90,
					FVector::Distance(r->conn[0]->node->get_point(), r->get_point()) / 2));
				auto bridge2 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
					r->conn[0]->node->get_point(), r->get_point(), r->get_point(), -90,
					FVector::Distance(r->conn[0]->node->get_point(), r->get_point()) / 2));
				add_conn(bridge1, bridge2);
				roads.Add(bridge1);
				roads.Add(bridge2);
			}
		}
	}
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

bool AMainTerrain::create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point)
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
		create_road_segment(roads, old_node, node, true, point_type::main_road);
		// add_conn(node, old_node);
		roads.AddUnique(node);
		old_node = node;
	}
	return true;
}

void AMainTerrain::shrink_roads()
{
	int road_points = roads.Num();
	int old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = roads.Num();
		for (int i = road_points - 1; i >= 0; i--)
		{
			if (roads[i]->conn.Num() < 2)
			{
				roads[i]->delete_me();
				roads.RemoveAt(i);
			}
		}
		road_points = roads.Num();
	}
}

void AMainTerrain::create_usual_roads()
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
			forward = 3;
			left = 9;
			right = 11;
			if (road_node->conn.Num() == 1)
			{
				if (rand() % 16 >= forward)
				{
					auto length = FVector::Distance(road_node->get_point(), road_node->conn[0]->node->get_point()) +
						(rand() % 40) - 20;
					if (length < min_road_length)
					{
						length = min_road_length;
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
						create_road_segment(add_road, road_node, new_node, false, point_type::road);
					}
				}
			}
			if (road_node->conn.Num() == 2 || road_node->conn.Num() == 1)
			{
				if (rand() % 16 >= left)
				{
					auto length = FVector::Distance(road_node->get_point(), road_node->conn[0]->node->get_point()) +
						(rand() % 40) - 20;
					if (length < min_road_length)
					{
						length = min_road_length;
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
						create_road_segment(add_road, road_node, new_node2, false, point_type::road);
					}
				}
				if (rand() % 16 >= right)
				{
					auto length = FVector::Distance(road_node->get_point(), road_node->conn[0]->node->get_point()) +
						(rand() % 40) - 20;
					if (length < min_road_length)
					{
						length = min_road_length;
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
						create_road_segment(add_road, road_node, new_node3, false, point_type::road);
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
				tick_road(roads[i]);
			};
		}
	}
}

TOptional<TSharedPtr<Node>> AMainTerrain::create_road_segment(TArray<TSharedPtr<Node>>& array,
															  TSharedPtr<Node> start_point, TSharedPtr<Node> end_point,
															  bool to_exect_point, point_type type)
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
			if (tries > 10 || FVector::Distance(start_point->get_point(), end_point->get_point()) > max_road_length)
			{
				break;
			}
			intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, array, false);
		}
		if (tries > 10 || FVector::Distance(start_point->get_point(), end_point->get_point()) > max_road_length ||
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

		auto river_intersection =
			AllGeometry::is_intersect_array_clear(start_point->get_point(), end_point->get_point(), river, true);
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

void AMainTerrain::point_shift(FVector& point)
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

void AMainTerrain::create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
							   float ExtrusionHeight)
{
	// ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	// RootComponent = ProceduralMesh;
	//
	// TArray<FVector> Vertices;
	// TArray<int32> Triangles;
	//
	// // Шаг 1: Добавляем вершины базовой фигуры и экструзированные вершины
	// int32 NumBaseVertices = BaseVertices.Num();
	//
	// // Добавляем вершины плоской фигуры (нижняя плоскость)
	// for (auto Vertex : BaseVertices)
	// {
	// 	Vertices.Add(Vertex->node); // Нижняя грань
	// }
	//
	// // Добавляем экструзированные вершины (верхняя плоскость)
	// for (auto Vertex : BaseVertices)
	// {
	// 	Vertices.Add(Vertex->node + FVector(0, 0, ExtrusionHeight)); // Верхняя грань
	// }
	//
	// // Шаг 2: Создаем треугольники для боковых граней
	// for (int32 i = 0; i < NumBaseVertices; i++)
	// {
	// 	int32 NextIndex = (i + 1) % NumBaseVertices; // Индекс следующей вершины для замыкания
	//
	// 	// Боковая грань (состоит из двух треугольников)
	// 	Triangles.Add(i); // Нижняя грань (текущая вершина)
	// 	Triangles.Add(i + NumBaseVertices); // Верхняя грань (текущая экструзированная вершина)
	// 	Triangles.Add(NextIndex); // Нижняя грань (следующая вершина)
	//
	// 	Triangles.Add(NextIndex); // Нижняя грань (следующая вершина)
	// 	Triangles.Add(i + NumBaseVertices); // Верхняя грань (текущая экструзированная вершина)
	// 	Triangles.Add(NextIndex + NumBaseVertices); // Верхняя грань (следующая экструзированная вершина)
	// }
	//
	// // Шаг 3: Создаем треугольники для нижней грани
	// for (int32 i = 1; i < NumBaseVertices - 1; i++)
	// {
	// 	Triangles.Add(0);
	// 	Triangles.Add(i);
	// 	Triangles.Add(i + 1);
	// }
	//
	// // Шаг 4: Создаем треугольники для верхней грани (перевёрнутые индексы, чтобы нормали смотрели вверх)
	// for (int32 i = 1; i < NumBaseVertices - 1; i++)
	// {
	// 	Triangles.Add(NumBaseVertices); // Начальная точка верхней грани
	// 	Triangles.Add(NumBaseVertices + i + 1);
	// 	Triangles.Add(NumBaseVertices + i);
	// }
	//
	// // Шаг 5: Создаем нормали, UV и другие параметры (опционально)
	// TArray<FVector> Normals;
	// TArray<FVector2D> UVs;
	// TArray<FLinearColor> VertexColors;
	// TArray<FProcMeshTangent> Tangents;
	//
	// // Добавляем заготовки для нормалей, UV и т.д.
	// for (int32 i = 0; i < Vertices.Num(); i++)
	// {
	// 	Normals.Add(FVector(0, 0, 1)); // Пример, что все нормали смотрят вверх
	// 	UVs.Add(FVector2D(0, 0)); // Заглушка UV
	// 	VertexColors.Add(FLinearColor::White); // Цвет вершин (белый)
	// 	Tangents.Add(FProcMeshTangent(1, 0, 0)); // Пример тангенса
	// }
	//
	// // Шаг 6: Создаем Mesh секцию
	// Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}


void AMainTerrain::get_closed_figures(TArray<TSharedPtr<Node>> lines)
{
	for (auto l : lines)
	{
		for (auto lconn : l->conn)
		{
			if (!lconn->figure->IsEmpty() || lconn->not_in_figure)
			{
				continue;
			}
			TSharedPtr<TArray<TSharedPtr<Point>>> figure_array = MakeShared<TArray<TSharedPtr<Point>>>();
			TArray<TSharedPtr<Conn>> conn_array;
			conn_array.Add(lconn);
			auto first_node = l;
			auto second_node = lconn->node;
			figure_array->Add(l->get_node());
			figure_array->Add(lconn->node->get_node());
			TSharedPtr<Node> rightest_node;
			TSharedPtr<Conn> this_conn;
			bool some_error = false;
			bool not_in_figure = false;
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
				if (smallest_angle == 360)
				{
					not_in_figure = true;
					break;
				}
				figure_array->Add(rightest_node->get_node());
				conn_array.Add(this_conn);
				if (figure_array->Num() > 500)
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
			if (not_in_figure)
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
				figures_array.Add(Block(*figure_array));
			}
		}
	}
}
void AMainTerrain::process_blocks(TArray<Block>& blocks)
{
	blocks.Sort([this](Block Item1, Block Item2) { return Item1.area > Item2.area; });
	blocks.RemoveAt(0);

	double royal_area = 0;
	bool royal_found = false;
	bool dock_found = false;
	for (auto& b : blocks)
	{
		if (b.get_type() == block_type::unknown && !dock_found)
		{
			for (auto r : river)
			{
				if (b.is_point_in_figure(r->get_node()))
				{
					dock_found = true;
					b.set_type(block_type::dock);
					break;
				}
			}
		}
		if (b.get_type() == block_type::unknown && !royal_found)
		{
			for (auto& p : b.figure)
			{
				if (FVector::Distance(p->point, center) < (x_size + y_size) / 10)
				{
					b.set_type(block_type::royal);
					royal_area += b.area;
					break;
				}
			}
			if (royal_area < 100000)
			{
				for (auto& p : b.figure)
				{
					if (p->type == block_type::royal)
					{
						b.set_type(block_type::royal);
						royal_area += b.area;
						break;
					}
				}
			}
			else
			{
				royal_found = true;
			}
		}
	}
}

void AMainTerrain::draw_all()
{
	FlushPersistentDebugLines(GetWorld());

	for (auto b : map_borders_array)
	{
		for (auto bconn : b->conn)
		{
			DrawDebugLine(GetWorld(), bconn->node->get_point(), b->get_point(), FColor::White, true, -1, 0, 20);
		}
	}

	for (auto r : river)
	{
		for (auto rconn : r->conn)
		{
			DrawDebugLine(GetWorld(), rconn->node->get_point(), r->get_point(), FColor::Blue, true, -1, 0, 10);
		}
	}
	for (int i = 0; i < roads.Num(); i++)
	{
		FColor color = FColor((255 / roads[i]->conn.Num() * i), 255, (255 / roads[i]->conn.Num() * i));

		// DrawDebugSphere(GetWorld(), roads[i]->get_point(), 4, 8, color, true, -1, 0, 1);

		for (int j = 0; j < roads[i]->conn.Num(); j++)
		{
			// auto color = FColor::Green;
			// if (FVector::Distance(rconn->node->get_point(), r->get_point()) > max_road_length)
			// {
			// 	color = FColor::Red;
			// }
			// if (r->get_type() == point_type::main_road && r->conn[i]->node->get_type() == point_type::main_road)
			// {
			// DrawDebugLine(GetWorld(), roads[i]->conn[j]->node->get_point(), roads[i]->get_point(), FColor::Green,
			// true, 			  -1, 0, 3);
			// }
			// else
			// {
			// 	DrawDebugLine(GetWorld(), r->conn[i]->node->get_point(), r->get_point(), color, true, -1, 0, 1);
			// }
		}
	}
	for (auto r : figures_array)
	{
		FColor color;
		int thickness = 1;
		auto figure_we_got = r.figure;
		for (int i = 1; i < figure_we_got.Num(); i++)
		{
			if (r.get_type() == block_type::dock)
			{
				color = FColor((255 / figure_we_got.Num() * i), (255 / figure_we_got.Num() * i), 255);
				thickness = 5;
				// DrawDebugSphere(GetWorld(), figure_we_got[i - 1]->point, 4, 8, color, true, -1, 0, 1);
			}
			else if (r.get_type() == block_type::royal)
			{
				color = FColor(255, (255 / figure_we_got.Num() * i), (255 / figure_we_got.Num() * i));
				thickness = 5;
			}
			else
			{
				color = FColor::Green;
				thickness = 1;
			}
			DrawDebugLine(GetWorld(), figure_we_got[i - 1]->point, figure_we_got[i]->point, color, true, -1, 0,
						  thickness);
		}
	}
}
