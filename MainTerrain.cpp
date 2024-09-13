#include "MainTerrain.h"

#include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() { PrimaryActorTick.bCanEverTick = true; }

void AMainTerrain::BeginPlay()
{
	Super::BeginPlay();

	create_terrain();
	// map_borders_array.Reset();
	tick_terrain();
	// map_lines_array.SetNum(4);
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
		for (auto& r : river)
		{
			tick_river(r);
		}
		// main_lines_array.Reset();
	}

	// for (auto r : river)
	// {
	// 	if (!r->prev.IsEmpty())
	// 	{
	// 		main_lines_array.Add(PointLine{
	// 			r->prev[0]->node, r->node, point_type::river
	// 		});
	// 	}
	// }

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

void AMainTerrain::tick_river(TSharedPtr<Node>& node)
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
	FVector all_point(0, 0, 0);
	double count = 0;
	for (int i = 0; i < node->conn.Num(); i++)
	{
		all_point += node->conn[i]->node;
		count++;
	}
	bool is_break = false;
	for (int i = 0; i < node->conn.Num(); i++)
	{
		if (is_break)
		{
			break;
		}

		if (
			FVector::Distance(node->node, node->conn[i]->node) > (y_size / 15))
		{
			node->node = all_point / count;
			break;
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
				if (FVector::Distance(node->node, node->conn[i]->node) > (y_size / 30) || (is_continuing &&
					AllGeometry::calculate_angle(node->conn[0]->node, node->node, node->conn[1]->node) < 155.0))
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
	Node map_node1 = Node(0, 0, 0);
	Node map_node2 = Node(0, y_size, 0);
	Node map_node3 = Node(x_size, y_size, 0);
	Node map_node4 = Node(x_size, 0, 0);
	map_node1.conn.Add(MakeShared<Node>(map_node2));
	map_node1.conn.Add(MakeShared<Node>(map_node4));
	map_node2.conn.Add(MakeShared<Node>(map_node1));
	map_node2.conn.Add(MakeShared<Node>(map_node3));
	map_node3.conn.Add(MakeShared<Node>(map_node2));
	map_node3.conn.Add(MakeShared<Node>(map_node4));
	map_node4.conn.Add(MakeShared<Node>(map_node3));
	map_node4.conn.Add(MakeShared<Node>(map_node1));
	map_borders_array.Add(MakeShared<Node>(map_node1));
	map_borders_array.Add(MakeShared<Node>(map_node2));
	map_borders_array.Add(MakeShared<Node>(map_node3));
	map_borders_array.Add(MakeShared<Node>(map_node4));
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
	FVector start_river_point(0, 0, 0);
	FVector end_river_point(0, y_size, 0);
	Node start_point((start_river_point + end_river_point) / 2);

	start_point.node = (start_river_point + end_river_point) / 2;
	FVector end_river = AllGeometry::create_segment_at_angle(FVector(0, 0, 0), FVector{0, y_size, 0},
	                                                         start_point.node, -90 + (rand() % 20),
	                                                         (rand() % 20 + 10) * (av_river_length)
	);
	Node end_point(end_river);
	create_guiding_river_segment(MakeShared<Node>(start_point), MakeShared<Node>(end_point));
}


void AMainTerrain::create_guiding_river_segment(TSharedPtr<Node> start_point,
                                                TSharedPtr<Node> end_point)
{
	river.AddUnique(start_point);
	bool is_ending = false;

	auto intersect_river = AllGeometry::is_intersect_array_clear(start_point, end_point, river, false);
	auto intersect_border = AllGeometry::is_intersect_array(start_point, end_point, map_borders_array, false);
	if (intersect_border.IsSet())
	{
		is_ending = true;
		end_point->node = intersect_border->Key;
	}
	else if (intersect_river.IsSet())
	{
		is_ending = true;
		end_point = intersect_river.GetValue();
	}

	TSharedPtr<Node> old_node = start_point;
	// river.Add(old_node);
	int dist_times = FVector::Distance(start_point->node, end_point->node) / (av_river_length);
	for (int i = 1; i <= dist_times; i++)
	{
		Node node;
		auto node_ptr = MakeShared<Node>(node);
		if (i != dist_times)
		{
			node_ptr->conn.Add(old_node);
			old_node->conn.Add(node_ptr);
			node_ptr->node = start_point->node + ((end_point->node - start_point->node) / dist_times * i);
		}
		else
		{
			end_point->conn.Add(old_node);
			old_node->conn.Add(end_point);
		}
		river.AddUnique(node_ptr);
		old_node = node_ptr;
	}
	if (!is_ending)
	{
		Node next_segment = Node(AllGeometry::create_segment_at_angle(start_point->node, end_point->node,
		                                                              end_point->node,
		                                                              -60 + (rand() % 120),
		                                                              (rand() % 20 + 10) * (av_river_length)));
		create_guiding_river_segment(end_point, MakeShared<Node>(next_segment));
		if (rand() % 4 >= 3)
		{
			Node next_segment1 = Node(AllGeometry::create_segment_at_angle(start_point->node, end_point->node,
			                                                               end_point->node,
			                                                               -60 + (rand() % 120),
			                                                               (rand() % 20 + 10) * (av_river_length)));
			create_guiding_river_segment(end_point, MakeShared<Node>(next_segment1));
		}
		if (rand() % 8 >= 3)
		{
			Node next_segment2 = Node(AllGeometry::create_segment_at_angle(start_point->node, end_point->node,
			                                                               end_point->node,
			                                                               -60 + (rand() % 120),
			                                                               (rand() % 20 + 10) * (av_river_length)));
			create_guiding_river_segment(end_point, MakeShared<Node>(next_segment2));
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
		if (rand() % 8 >= 6 && r->conn.Num() == 2)
		{
			Node bridge1(AllGeometry::create_segment_at_angle(r->conn[0]->node, r->node, r->node, 90,
			                                                  FVector::Dist(r->conn[0]->node, r->node) / 2));
			Node bridge2(AllGeometry::create_segment_at_angle(r->conn[0]->node, r->node, r->node, -90,
			                                                  FVector::Dist(r->conn[0]->node, r->node) / 2));
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
	// for (auto& it : map_lines_array)
	// {
	// 	FColor color;
	// 	double point_radius = 10.0f;
	// 	switch (it.type)
	// 	{
	// 	case main:
	// 		color = FColor::White;
	// 		point_radius = 25.0f;
	// 	// DrawDebugSphere(GetWorld(), it.line_begin, 1, 8, color, true);
	// 	// DrawDebugSphere(GetWorld(), it.line_end, 1, 8, color, true);
	// 		break;
	// 	case point_type::road:
	// 		color = FColor::Green;
	// 		point_radius = 10.0f;
	// 		DrawDebugSphere(GetWorld(), it.line_begin, 20, 8, color, true);
	// 		DrawDebugSphere(GetWorld(), it.line_end, 20, 8, color, true);
	// 		break;
	// 	case point_type::river:
	// 		color = FColor::Blue;
	// 		point_radius = 20.0f;
	// 		DrawDebugSphere(GetWorld(), it.line_begin, 1, 8, color, true);
	// 		DrawDebugSphere(GetWorld(), it.line_end, 1, 8, color, true);
	// 		break;
	// 	}
	//
	// 	// DrawDebugSphere(GetWorld(), it.line_begin, point_radius, 8, color,
	// 	// true); DrawDebugSphere(GetWorld(), it.line_end, point_radius, 8, color,
	// 	// true);
	// 	DrawDebugLine(GetWorld(), it.line_begin, it.line_end, color, true, -1,
	// 	              0,
	// 	              10);
	// }
	for (auto b : map_borders_array)
	{
		for (auto bconn : b->conn)
		{
			DrawDebugLine(GetWorld(), bconn->node, b->node, FColor::White, true,
			              -1, 0, 20);
		}
	}

	for (auto r : river)
	{
		for (auto rconn : r->conn)
		{
			DrawDebugLine(GetWorld(), rconn->node, r->node, FColor::Blue, true,
			              -1, 0, 10);
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


FVector AllGeometry::create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
                                             const FVector& line_beginPoint, double angle_in_degrees, double length)
{
	double Dx = line_end.X - line_begin.X;
	double Dy = line_end.Y - line_begin.Y;

	double AngleInRadians = FMath::DegreesToRadians(angle_in_degrees);

	double NewX = line_beginPoint.X + (Dx * FMath::Cos(AngleInRadians) - Dy * FMath::Sin(AngleInRadians));
	double NewY = line_beginPoint.Y + (Dx * FMath::Sin(AngleInRadians) + Dy * FMath::Cos(AngleInRadians));

	FVector line_endPointBL{NewX, NewY, line_end.Z};

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
