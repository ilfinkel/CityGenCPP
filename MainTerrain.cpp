#include "MainTerrain.h"

#include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() { PrimaryActorTick.bCanEverTick = true; }

void AMainTerrain::BeginPlay()
{
	Super::BeginPlay();

	create_terrain();
	main_lines_array.Reset();
	tick_terrain();
	map_lines_array.SetNum(4);
	int old_nodes = 0;
	while (roads.Num() != old_nodes)
	{
		old_nodes = roads.Num();
		create_usual_roads();
	}
	// old_nodes = 0;
	// while (roads.Num() != old_nodes)
	// {
	// 	old_nodes = roads.Num();
	// 	for(int i = roads.Num()-1; i >= 0 ; i--)
	// 	{
	// 		if(roads[i]->conn.Num()<2)
	// 		{
	// 			roads[i]= nullptr;
	// 		}
	// 	}
	// }
	draw_all();
}

// Called every frame
void AMainTerrain::Tick(float DeltaTime)
{
	// Super::Tick(DeltaTime);
	// create_usual_roads();
	// draw_all();
}

void AMainTerrain::tick_terrain()
{
	for (int iter = 0; iter < 100; iter++)
	{
		main_lines_array.Reset();
		tick_river(river[0]);
	}

	for (auto r : river)
	{
		if (!r->prev.IsEmpty())
		{
			main_lines_array.Add(PointLine{
				r->prev[0]->node, r->node, point_type::river
			});
		}
	}

	for (int iter = 0; iter < 1000; iter++)
	{
		for (auto& r : roads)
		{
			r->used = false;
		}

		for (auto& road_center : road_centers)
		{
			road_center->used = true;
		}

		for (auto& r : roads)
		{
			tick_road(r);
		}
	}

	// for (auto r : roads)
	// {
	// 	for (auto rconn : r->conn)
	// 	{
	// 		main_lines_array.Add(PointLine{
	// 			rconn->node, r->node, point_type::road
	// 		});
	// 	}
	// }
}

void AMainTerrain::tick_river(TSharedPtr<RiverNode>& node)
{
	point_shift(node->node);

	if ((node->node.X) < 0)
	{
		node->node.X = 0;
	}
	if ((node->node.Y) < 0)
	{
		node->node.Y = 0;
	}
	if ((node->node.X) > x_size)
	{
		node->node.X = x_size;
	}
	if ((node->node.Y) > y_size)
	{
		node->node.Y = y_size;
	}
	if (!node->prev.IsEmpty() && !node->next.IsEmpty())
	{
		FVector point_prev(0, 0, 0);
		FVector point_next(0, 0, 0);
		double count_prev = 0;
		double count_next = 0;
		for (int i = 0; i < node->prev.Num(); i++)
		{
			point_prev += node->prev[i]->node;
			count_prev++;
		}
		for (int i = 0; i < node->next.Num(); i++)
		{
			point_next += node->next[i]->node;
			count_next++;
		}
		bool is_break = false;
		for (int i = 0; i < node->prev.Num(); i++)
		{
			if (is_break)
			{
				break;
			}
			for (int j = 0; j < node->next.Num(); j++)
			{
				if (
					(FVector::Distance(node->node, node->prev[i]->node) > (
							y_size / 15) ||
						FVector::Distance(node->node, node->next[j]->node) > (
							y_size / 15)))
				{
					node->node = (point_prev + point_next) / (count_prev +
						count_next);
					is_break = true;
					break;
				}
			}
		}
	}
	if (!node->next.IsEmpty())
	{
		for (auto& next_node : node->next)
		{
			tick_river(next_node);
		}
	}
}

void AMainTerrain::tick_road(TSharedPtr<Node>& node)
{
	if (node->used)
	{
		return;
	}
	auto backup_node = node;
	point_shift(node->node);

	if (AllGeometry::is_intersect_array(node, backup_node, river, true).IsSet())
	{
		node = backup_node;
		return;
	}

	if ((node->node.X) < 0)
	{
		node->node.X = 0;
	}
	if ((node->node.Y) < 0)
	{
		node->node.Y = 0;
	}
	if ((node->node.X) > x_size)
	{
		node->node.X = x_size;
	}
	if ((node->node.Y) > y_size)
	{
		node->node.Y = y_size;
	}
	if (!node->conn.IsEmpty())
	{
		FVector middle_point(0, 0, 0);
		for (auto p : node->conn)
		{
			middle_point += p->node;
		}
		middle_point /= node->conn.Num();

		bool is_continuing = (node->conn.Num() == 2);
		for (int i = 0; i < node->conn.Num(); i++)
		{
			for (int j = 1; j < node->conn.Num(); j++)
			{
				if (i == j) { continue; }
				if (FVector::Distance(node->node, node->conn[i]->node) > (y_size
						/ 30)
					|| (is_continuing &&
						AllGeometry::calculate_angle(
							node->conn[0]->node, node->node,
							node->conn[1]->node) <
						155.0))
				{
					node->node = middle_point;
					return;
				}
			}
		}
	}
	// node->used = true;
}

void AMainTerrain::create_terrain()
{
	map_lines_array.Add(
		PointLine{FVector{0, 0, 0}, FVector{0, y_size, 0}, main});
	map_lines_array.Add(PointLine{
		FVector{0, y_size, 0}, FVector{x_size, y_size, 0}, main
	});
	map_lines_array.Add(PointLine{
		FVector{x_size, y_size, 0},
		FVector{x_size, 0, 0}, main
	});
	map_lines_array.Add(
		PointLine{FVector{x_size, 0, 0}, FVector{0, 0, 0}, main});
	double points_count = 81;
	double av_size = (x_size + y_size) / 2;

	weighted_points.Empty();

	const double point_row_counter = sqrt(points_count);
	for (double x = 1; x < point_row_counter; x++)
	{
		for (int y = 1; y < point_row_counter; y++)
		{
			weighted_points.Add(WeightedPoint{
				FVector(
					static_cast<int>(x_size / point_row_counter * x + (rand() % (static_cast<int>(x_size /
						point_row_counter / 2))) - (x_size / point_row_counter / 4)),
					static_cast<int>(y_size / point_row_counter * y) + (rand() % (static_cast<int>(x_size /
						point_row_counter / 2))) - (x_size / point_row_counter / 4), 0),
				(rand() % static_cast<int>(av_size / point_row_counter / 2)) + (av_size /
					point_row_counter / 3)
			});
		}
	}
	create_guiding_rivers();
	create_guiding_roads();
}

void AMainTerrain::create_guiding_rivers()
{
	PointLine start_line(FVector(0, 0, 0), FVector(0, y_size, 0));
	RiverNode start_point;

	start_point.node = (start_line.line_begin + start_line.line_end) / 2;
	PointLine starting_river = AllGeometry::create_segment_at_angle(
		start_line, start_point.node,
		-90 + (rand() % 20), (rand() % 20) * (av_river_length),
		point_type::river);
	// river.Add(MakeShared<Node>(start_point));
	create_guiding_river_segment(starting_river,
	                             MakeShared<RiverNode>(start_point));
}

void AMainTerrain::create_guiding_river_segment(
	PointLine& starting_river, const TSharedPtr<RiverNode>& start_point)
{
	bool is_ending = false;
	auto intersect_border = AllGeometry::is_intersect_array(starting_river, map_lines_array, false);
	map_lines_array.Add(starting_river);
	TSharedPtr<RiverNode> next_river;
	if (intersect_border.IsSet())
	{
		for (auto& r : river)
		{
			if (!r->prev.IsEmpty())
			{
				for (auto& rprev : r->prev)
				{
					PointLine pl(rprev->node, r->node);
					if (AllGeometry::is_intersect(starting_river, pl, false))
					{
						is_ending = true;
						next_river = r;
						break;
					}
				}
				if (is_ending == true)
				{
					break;
				}
			}
		}
		is_ending = true;
		starting_river.line_end = intersect_border->Key;
	}

	TSharedPtr<RiverNode> old_node = start_point;
	// river.Add(old_node);
	auto dist_times = FVector::Distance(starting_river.line_begin, starting_river.line_end) / (av_river_length);
	for (int i = 1; i <= dist_times; i++)
	{
		RiverNode node;
		auto node_ptr = MakeShared<RiverNode>(node);
		node_ptr->prev.Add(old_node);
		node_ptr->node = starting_river.line_begin + ((starting_river.line_end - starting_river.line_begin) / dist_times
			* i);

		old_node->next.Add(node_ptr);
		river.Add(node_ptr);
		old_node = node_ptr;
	}
	if (!is_ending)
	{
		auto next_segment = AllGeometry::create_segment_at_angle(starting_river, starting_river.line_end,
		                                                         -60 + (rand() % 120),
		                                                         (rand() % 20 + 20) * (av_river_length),
		                                                         point_type::river);
		create_guiding_river_segment(next_segment, old_node);
		if (rand() % 4 >= 3)
		{
			auto next_segment1 = AllGeometry::create_segment_at_angle(starting_river, starting_river.line_end,
			                                                          -60 + (rand() % 120),
			                                                          (rand() % 20 + 20) * (av_river_length),
			                                                          point_type::river);
			create_guiding_river_segment(next_segment1, old_node);
		}
		if (rand() % 8 >= 3)
		{
			auto next_segment2 = AllGeometry::create_segment_at_angle(starting_river, starting_river.line_end,
			                                                          -60 + (rand() % 120),
			                                                          (rand() % 20 + 20) * (av_river_length),
			                                                          point_type::river);
			create_guiding_river_segment(next_segment2, old_node);
		}
	}
	else if (next_river.IsValid())
	{
		next_river->prev.Add(old_node);
		old_node->next.Add(next_river);
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
			auto x_val = x_size / point_row_counter * x + (rand() % (static_cast<int>(x_size / point_row_counter / 2)))
				- (x_size / point_row_counter / 4);
			auto y_val = y_size / point_row_counter * y + (rand() % (static_cast<int>(x_size / point_row_counter / 2)))
				- (x_size / point_row_counter / 4);
			roads.Add(MakeShared<Node>(Node{FVector(x_val, y_val, 0)}));
		}
	}
	for (auto& r : roads)
	{
		for (int iter = 0; iter < 1000; iter++)
		{
			point_shift(r->node);
		}
	}
	TArray<TTuple<double, TSharedPtr<Node>>> weighted_road_centers;
	for (auto& r : roads)
	{
		if (FVector::Distance(r->node, FVector{x_size / 2, y_size / 2, 0}) < (y_size / 3))
		{
			bool is_break = false;
			for (auto riv : river)
			{
				if (FVector::Distance(r->node, riv->node) < river_road_distance)
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
				if (FVector::Distance(r->node, w.point) < w.weight)
				{
					weight += (w.weight - FVector::Distance(r->node, w.point));
				}
			}
			weighted_road_centers.Add(
				TTuple<double, TSharedPtr<Node>>{weight, r});
		}
	}

	Algo::Sort(weighted_road_centers,
	           [](const TTuple<double, TSharedPtr<Node>>& A,
	              const TTuple<double, TSharedPtr<Node>>& B)
	           {
		           return A.Get<0>() < B.Get<0>();
	           });
	roads.Reset();
	// road.Add(weighted_road_centers[0].Value);
	for (int i = 1; i < weighted_road_centers.Num(); i++)
	{
		bool is_break = false;
		for (auto r : roads)
		{
			if (FVector::Distance(weighted_road_centers[i].Value->node, r->node)
				< (y_size / 7))
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
	// расставляем мосты
	for (auto r : river)
	{
		if (rand() % 8 >= 6 && r->prev.Num() == 1)
		{
			Node bridge1(AllGeometry::create_segment_at_angle(r->prev[0]->node, r->node, r->node, 90,
			                                                  FVector::Dist(r->prev[0]->node, r->node) / 2));
			Node bridge2(AllGeometry::create_segment_at_angle(r->prev[0]->node, r->node, r->node, -90,
			                                                  FVector::Dist(r->prev[0]->node, r->node) / 2));
			bridge1.conn.Add(MakeShared<Node>(bridge2));
			bridge2.conn.Add(MakeShared<Node>(bridge1));
			roads.Add(MakeShared<Node>(bridge1));
			roads.Add(MakeShared<Node>(bridge2));
		}
	}
	for (auto r : roads)
	{
		weighted_points.Add(WeightedPoint{r->node, -y_size / 7});
	}

	// road.SetNum(());
	for (auto& r : roads)
	{
		road_centers.Add(r);
	}

	for (auto& r : road_centers)
	{
		r->type = point_type::main_road;
	}
	// road.Sort([this](auto Item1, auto Item2) {
	//   return FMath::FRand() < 0.5f;
	// });
	for (int i = roads.Num() - 2; i >= 0; i--)
	{
		TArray<TSharedPtr<Node>> local_road = TArray<TSharedPtr<Node>>(
			&roads.GetData()[0], i);

		local_road.Sort(
			[i, this](TSharedPtr<Node> Item1, TSharedPtr<Node> Item2)
			{
				return FVector::Distance(roads[i + 1]->node, Item1->node) < FVector::Distance(
					roads[i + 1]->node, Item2->node);
			});
		for (int j = 0; j < 3 && j < local_road.Num() - 1; j++)
		{
			create_guiding_road_segment(roads[i + 1], local_road[j]);
		}
	}
}

void AMainTerrain::create_guiding_road_segment(
	TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point)
{
	if (AllGeometry::is_intersect_array(start_point, end_point, river, false).
		IsSet())
	{
		return;
	}

	TSharedPtr<Node> old_node = start_point;
	// river.Add(old_node);
	int dist_times = FVector::Distance(start_point->node, end_point->node) / (av_road_length);
	for (int i = 1; i <= dist_times; i++)
	{
		Node node(0, 0, 0);
		node.type = point_type::main_road;
		TSharedPtr<Node> node_ptr = MakeShared<Node>(node);
		node_ptr->conn.Add(old_node);
		node_ptr->node = start_point->node + ((end_point->node - start_point->node) / dist_times * i);
		auto inter_node = AllGeometry::is_intersect_array_clear(old_node, node_ptr, roads, false);
		if (inter_node.IsSet())
		{
			node_ptr = inter_node.GetValue();
		}
		if (i == dist_times)
		{
			node_ptr = end_point;
		}

		old_node->conn.Add(node_ptr);
		roads.AddUnique(node_ptr);
		old_node = node_ptr;
	}
}

void AMainTerrain::create_usual_roads()
{
	roads.Sort([this](auto Item1, auto Item2)
	{
		return FMath::FRand() < 0.5f;
	});
	TArray<TSharedPtr<Node>> add_road;
	for (auto r : roads)
	{
		if (!r->used)
		{
			if (r->conn.Num() == 1)
			{
				if (rand() % 8 >= 3)
				{
					auto length = FVector::Distance(r->node, r->conn[0]->node) + (rand() % 20) - 10;
					if (length < min_road_length) { length = min_road_length; }
					if (length > max_road_length) { length = max_road_length; }
					double angle_in_degrees = (rand() % 10) - 5;
					auto line1 = AllGeometry::create_segment_at_angle(
						r->conn[0]->node, r->node, r->node, angle_in_degrees, length);

					TSharedPtr<Node> new_node = MakeShared<Node>(line1);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->node, line1) < rc->conn.Num() * (y_size / 14))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						create_usual_road_segment(add_road, r, new_node);
					}
				}
			}
			if (r->conn.Num() == 2 || r->conn.Num() == 1)
			{
				if (rand() % 16 >= 11)
				{
					auto length = FVector::Distance(r->node, r->conn[0]->node) + (rand() % 20) - 10;
					if (length < min_road_length) { length = min_road_length; }
					if (length > max_road_length) { length = max_road_length; }
					double angle_in_degrees = 90 + (rand() % 10) - 5;
					auto line2 = AllGeometry::create_segment_at_angle(
						r->conn[0]->node, r->node, r->node, angle_in_degrees, length);
					TSharedPtr<Node> new_node2 = MakeShared<Node>(line2);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->node, line2) < rc->conn.Num() * (y_size / 14))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						create_usual_road_segment(add_road, r, new_node2);
					}
				}
				if (rand() % 16 >= 8)
				{
					auto length = FVector::Distance(r->node, r->conn[0]->node) + (rand() % 20) - 10;
					if (length < min_road_length) { length = min_road_length; }
					if (length > max_road_length) { length = max_road_length; }
					double angle_in_degrees = -90 + (rand() % 10) - 5;
					auto line3 = AllGeometry::create_segment_at_angle(
						r->conn[0]->node, r->node, r->node, angle_in_degrees, length);
					TSharedPtr<Node> new_node3 = MakeShared<Node>(line3);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->node, line3) < rc->conn.Num() * (y_size / 14))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						create_usual_road_segment(add_road, r, new_node3);
					}
				}
			}
		}

		r->used = true;
	}
	roads += add_road;
	for (int count = 0; count < 2; count++)
	{
		for (int i = 0; i < roads.Num(); i++)
		{
			if (!roads[i]->used)
			{
				tick_road(roads[i]);
			};
		}
	}
}

void AMainTerrain::create_usual_road_segment(TArray<TSharedPtr<Node>>& array,
                                             TSharedPtr<Node> start_point, TSharedPtr<Node> end_point)
{
	if ((end_point->node.X) < 0)
	{
		end_point->node.X = 0;
		end_point->used = true;
	}
	if ((end_point->node.Y) < 0)
	{
		end_point->node.Y = 0;
		end_point->used = true;
	}
	if ((end_point->node.X) > x_size)
	{
		end_point->node.X = x_size;
		end_point->used = true;
	}
	if ((end_point->node.Y) > y_size)
	{
		end_point->node.Y = y_size;
		end_point->used = true;
	}
	auto new_array = array;
	new_array += roads;
	auto intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, new_array, false);
	if (intersection.IsSet())
	{
		end_point = intersection.GetValue();
	}


	auto river_intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, river, false);
	if (!river_intersection.IsSet())
	{
		start_point->conn.Add(end_point);
		end_point->conn.Add(start_point);
		array.Add(end_point);
	}
}


void AMainTerrain::point_shift(FVector& node)
{
	FVector bias(0, 0, 0);
	double biggest_weight = 0;
	for (int j = 0; j < weighted_points.Num(); j++)
	{
		double length = FVector::Distance(node, weighted_points[j].point);

		if (length < abs(weighted_points[j].weight) && biggest_weight < weighted_points[j].weight - length)
		{
			biggest_weight = weighted_points[j].weight - length;
			bias = ((node - weighted_points[j].point) / length) * (weighted_points[j].weight - length) / 50;
		}
	}
	node += bias;
}

void AMainTerrain::draw_all()
{
	FlushPersistentDebugLines(GetWorld());

	// for (auto& it : main_lines_array) {
	//   FColor color;
	//   double point_radius = 10.0f;
	//   switch (it.type) {
	//     case point_type::main:
	//       color = FColor::White;
	//       point_radius = 25.0f;
	//       // DrawDebugSphere(GetWorld(), it.line_begin, 1, 8, color, true);
	//       // DrawDebugSphere(GetWorld(), it.line_end, 1, 8, color, true);
	//       break;
	//     case point_type::road:
	//       color = FColor::Green;
	//       point_radius = 10.0f;
	//       // DrawDebugSphere(GetWorld(), it.line_begin, 20, 8, color, true);
	//       // DrawDebugSphere(GetWorld(), it.line_end, 20, 8, color, true);
	//       break;
	//     case point_type::river:
	//       color = FColor::Blue;
	//       point_radius = 20.0f;
	//       // DrawDebugSphere(GetWorld(), it.line_begin, 40, 1, color, true);
	//       // DrawDebugSphere(GetWorld(), it.line_end, 40, 1, color, true);
	//       break;
	//   }
	//   // DrawDebugSphere(GetWorld(), it.line_begin, point_radius, 8, color,
	//   // true); DrawDebugSphere(GetWorld(), it.line_end, point_radius, 8, color,
	//   // true);
	//   DrawDebugLine(GetWorld(), it.line_begin, it.line_end, color, true, -1, 0,
	//                 10);
	// }
	//
	for (auto& it : map_lines_array)
	{
		FColor color;
		double point_radius = 10.0f;
		switch (it.type)
		{
		case main:
			color = FColor::White;
			point_radius = 25.0f;
		// DrawDebugSphere(GetWorld(), it.line_begin, 1, 8, color, true);
		// DrawDebugSphere(GetWorld(), it.line_end, 1, 8, color, true);
			break;
		case point_type::road:
			color = FColor::Green;
			point_radius = 10.0f;
			DrawDebugSphere(GetWorld(), it.line_begin, 20, 8, color, true);
			DrawDebugSphere(GetWorld(), it.line_end, 20, 8, color, true);
			break;
		case point_type::river:
			color = FColor::Blue;
			point_radius = 20.0f;
			DrawDebugSphere(GetWorld(), it.line_begin, 1, 8, color, true);
			DrawDebugSphere(GetWorld(), it.line_end, 1, 8, color, true);
			break;
		}

		// DrawDebugSphere(GetWorld(), it.line_begin, point_radius, 8, color,
		// true); DrawDebugSphere(GetWorld(), it.line_end, point_radius, 8, color,
		// true);
		DrawDebugLine(GetWorld(), it.line_begin, it.line_end, color, true, -1,
		              0,
		              10);
	}
	for (auto r : river)
	{
		for (auto rprev : r->prev)
		{
			DrawDebugLine(GetWorld(), rprev->node, r->node, FColor::Blue, true,
			              -1, 0,
			              10);
		}
	}

	for (auto r : roads)
	{
		for (auto rconn : r->conn)
		{
			if (r->type == point_type::main_road && rconn->type == point_type::main_road)
			{
				DrawDebugLine(GetWorld(), rconn->node, r->node, FColor::Green, true,
				              -1, 0, 9);
			}
			DrawDebugLine(GetWorld(), rconn->node, r->node, FColor::Green, true,
			              -1, 0, 5);
		}
		// if (r->used)
		// {
		// 	DrawDebugSphere(GetWorld(), r->node, 8, 8, FColor::White, true);
		// }
	}
	// map_lines_array.Empty();
}

void AllGeometry::create_main_line(PointLine line)
{
}


TOptional<FVector> AllGeometry::is_intersect(const PointLine& line1,
                                             const PointLine& line2,
                                             bool is_opened = false)
{
	double dx1 = line1.line_end.X - line1.line_begin.X;
	double dy1 = line1.line_end.Y - line1.line_begin.Y;
	double dx2 = line2.line_end.X - line2.line_begin.X;
	double dy2 = line2.line_end.Y - line2.line_begin.Y;

	double det = dx1 * dy2 - dx2 * dy1;

	if (std::abs(det) < 1e-6)
	{
		return TOptional<FVector>();
	}

	double t1 = ((line2.line_begin.X - line1.line_begin.X) * dy2 -
			(line2.line_begin.Y - line1.line_begin.Y) * dx2) /
		det;
	double t2 = ((line2.line_begin.X - line1.line_begin.X) * dy1 -
			(line2.line_begin.Y - line1.line_begin.Y) * dx1) /
		det;

	if (t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0)
	{
		FVector intersectionPoint(line1.line_begin.X + t1 * dx1,
		                          line1.line_begin.Y + t1 * dy1, 0);
		if (is_opened)
		{
			UE_LOG(LogTemp, Warning, TEXT("intersected!"));
			UE_LOG(LogTemp, Warning, TEXT("Point: %f,%f "), intersectionPoint.X,
			       intersectionPoint.Y);
			return intersectionPoint;
		}
		if (!is_opened && (FVector::Distance(intersectionPoint,
		                                     line2.line_begin) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line2.line_end) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line1.line_begin) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line1.line_end) >
			TNumericLimits<double>::Min()))
		{
			return intersectionPoint;
		}
	}

	return TOptional<FVector>(); // No collision
}

TOptional<FVector> AllGeometry::is_intersect(FVector line1_begin,
                                             FVector line1_end,
                                             FVector line2_begin,
                                             FVector line2_end, bool is_opened)
{
	double dx1 = line1_end.X - line1_begin.X;
	double dy1 = line1_end.Y - line1_begin.Y;
	double dx2 = line2_end.X - line2_begin.X;
	double dy2 = line2_end.Y - line2_begin.Y;

	double det = dx1 * dy2 - dx2 * dy1;

	if (std::abs(det) < 1e-6)
	{
		return TOptional<FVector>();
	}

	double t1 = ((line2_begin.X - line1_begin.X) * dy2 -
			(line2_begin.Y - line1_begin.Y) * dx2) /
		det;
	double t2 = ((line2_begin.X - line1_begin.X) * dy1 -
			(line2_begin.Y - line1_begin.Y) * dx1) /
		det;

	if (t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0)
	{
		FVector intersectionPoint(line1_begin.X + t1 * dx1,
		                          line1_begin.Y + t1 * dy1, 0);
		if (is_opened)
		{
			UE_LOG(LogTemp, Warning, TEXT("intersected!"));
			UE_LOG(LogTemp, Warning, TEXT("Point: %f,%f "), intersectionPoint.X,
			       intersectionPoint.Y);
			return intersectionPoint;
		}
		if (!is_opened && (FVector::Distance(intersectionPoint, line2_begin) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line2_end) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line1_begin) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line1_end) >
			TNumericLimits<double>::Min()))
		{
			return intersectionPoint;
		}
	}

	return TOptional<FVector>();
}

TOptional<TTuple<FVector, PointLine>> AllGeometry::is_intersect_array(
	const PointLine line1, const TArray<PointLine> lines,
	bool is_opened = false)
{
	PointLine point_line(FVector(0, 0, 0), FVector(0, 0, 0), main);
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		TOptional<FVector> int_point = is_intersect(line1, line, is_opened);
		if (int_point.IsSet())
		{
			double dist_to_line =
				FVector::Dist(line1.line_begin, int_point.GetValue());
			if (dist_to_line < dist)
			{
				point_line = line;
				dist = dist_to_line;
				intersect_point_final = int_point.GetValue();
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, PointLine>>();
	}
	TTuple<FVector, PointLine> final_tuple{intersect_point_final, point_line};
	return final_tuple;
}

TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>
AllGeometry::is_intersect_array(TSharedPtr<Node> line1_begin,
                                TSharedPtr<Node> line1_end,
                                const TArray<TSharedPtr<Node>> lines,
                                bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point = is_intersect(
				line1_begin->node, line1_end->node, line->node, conn->node, is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line1_begin->node, int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{line, conn};

					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{intersect_point_final, point_line};
	return final_tuple;
}

TOptional<TTuple<FVector, TTuple<TSharedPtr<RiverNode>, TSharedPtr<RiverNode>>>>
AllGeometry::is_intersect_array(TSharedPtr<Node> line1_begin,
                                TSharedPtr<Node> line1_end,
                                const TArray<TSharedPtr<RiverNode>> lines,
                                bool is_opened)
{
	TTuple<TSharedPtr<RiverNode>, TSharedPtr<RiverNode>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->prev)
		{
			TOptional<FVector> int_point = is_intersect(
				line1_begin->node, line1_end->node, line->node, conn->node,
				is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line1_begin->node, int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<RiverNode>, TSharedPtr<RiverNode>>{
						line, conn
					};


					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, TTuple<TSharedPtr<RiverNode>, TSharedPtr<RiverNode>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<RiverNode>, TSharedPtr<RiverNode>>> final_tuple
		{intersect_point_final, point_line};
	return final_tuple;
}

TOptional<TSharedPtr<Node>> AllGeometry::is_intersect_array_clear(TSharedPtr<Node> line1_begin,
                                                                  TSharedPtr<Node> line1_end,
                                                                  const TArray<TSharedPtr<Node>> lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<TSharedPtr<Node>>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->node) < FVector::Dist(
		       inter_segment->Key, inter_segment->Value.Value->node)
		       ? inter_segment->Value.Key
		       : inter_segment->Value.Value;
}

TOptional<TSharedPtr<RiverNode>> AllGeometry::is_intersect_array_clear(TSharedPtr<Node> line1_begin,
                                                                       TSharedPtr<Node> line1_end,
                                                                       const TArray<TSharedPtr<RiverNode>> lines,
                                                                       bool is_opened)
{
	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<TSharedPtr<RiverNode>>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->node) < FVector::Dist(
		       inter_segment->Key, inter_segment->Value.Value->node)
		       ? inter_segment->Value.Key
		       : inter_segment->Value.Value;
}


PointLine AllGeometry::create_segment_at_angle(const PointLine& BaseSegment, const FVector& line_beginPoint,
                                               double angle_in_degrees, double length, point_type p_type)
{
	double Dx = BaseSegment.line_end.X - BaseSegment.line_begin.X;
	double Dy = BaseSegment.line_end.Y - BaseSegment.line_begin.Y;

	double AngleInRadians = FMath::DegreesToRadians(angle_in_degrees);

	double NewX = line_beginPoint.X + (Dx * FMath::Cos(AngleInRadians) - Dy * FMath::Sin(AngleInRadians));
	double NewY = line_beginPoint.Y + (Dx * FMath::Sin(AngleInRadians) + Dy * FMath::Cos(AngleInRadians));

	FVector line_endPointBL{NewX, NewY, BaseSegment.line_end.Z};

	PointLine NewSegmentBL(line_beginPoint, line_endPointBL);

	Dx = NewSegmentBL.line_end.X - NewSegmentBL.line_begin.X;
	Dy = NewSegmentBL.line_end.Y - NewSegmentBL.line_begin.Y;

	NewX = line_beginPoint.X + Dx * length / NewSegmentBL.length();
	NewY = line_beginPoint.Y + Dy * length / NewSegmentBL.length();

	FVector line_endPoint(NewX, NewY, BaseSegment.line_end.Z);
	PointLine NewSegment(line_beginPoint, line_endPoint);
	NewSegment.type = p_type;

	return NewSegment;
}

FVector AllGeometry::create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
                                             const FVector& line_beginPoint, double angle_in_degrees, double length)
{
	double Dx = line_end.X - line_begin.X;
	double Dy = line_end.Y - line_begin.Y;

	double AngleInRadians = FMath::DegreesToRadians(angle_in_degrees);

	double NewX = line_beginPoint.X + (Dx * FMath::Cos(AngleInRadians) - Dy * FMath::Sin(AngleInRadians));
	double NewY = line_beginPoint.Y + (Dx * FMath::Sin(AngleInRadians) + Dy * FMath::Cos(AngleInRadians));

	FVector line_endPointBL{NewX, NewY, line_end.Z};

	PointLine NewSegmentBL(line_beginPoint, line_endPointBL);
	auto seg_length = FVector::Dist(line_beginPoint, line_endPointBL);
	Dx = line_endPointBL.X - line_beginPoint.X;
	Dy = line_endPointBL.Y - line_beginPoint.Y;

	NewX = line_beginPoint.X + Dx * length / seg_length;
	NewY = line_beginPoint.Y + Dy * length / seg_length;

	FVector line_endPoint(NewX, NewY, line_end.Z);

	return line_endPoint;
}

double AllGeometry::calculate_angle(const FVector A, const FVector B,
                                    const FVector C)
{
	FVector BA = A - B;
	FVector BC = C - B;
	BA.Normalize();
	BC.Normalize();
	double CosTheta = FVector::DotProduct(BA, BC);
	CosTheta = FMath::Clamp(CosTheta, -1.0f, 1.0f);
	double AngleRadians = FMath::Acos(CosTheta);
	double AngleDegrees = FMath::RadiansToDegrees(AngleRadians);
	return AngleDegrees;
}
