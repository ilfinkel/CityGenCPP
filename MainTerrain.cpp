#include "MainTerrain.h"


// #include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() :
	BaseMaterial(nullptr), WaterMaterial(nullptr), DocsMaterial(nullptr), RoyalMaterial(nullptr),
	ResidenceMaterial(nullptr), LuxuryMaterial(nullptr), SlumsMaterial(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}
void AMainTerrain::OnMouseOver(UPrimitiveComponent* Component)
{
	if (BaseMaterial)
	{
		Component->SetMaterial(0, BaseMaterial);
	}
}
void AMainTerrain::OnMouseOutRoyal(UPrimitiveComponent* Component)
{
	if (Component && RoyalMaterial)
	{
		Component->SetMaterial(0, RoyalMaterial);
	}
}
void AMainTerrain::OnMouseOutDock(UPrimitiveComponent* Component)
{
	if (Component && DocsMaterial)
	{
		Component->SetMaterial(0, DocsMaterial);
	}
}
void AMainTerrain::OnMouseOutLuxury(UPrimitiveComponent* Component)
{
	if (Component && LuxuryMaterial)
	{
		Component->SetMaterial(0, LuxuryMaterial);
	}
}
void AMainTerrain::OnMouseOutResidential(UPrimitiveComponent* Component)
{
	if (Component && ResidenceMaterial)
	{
		Component->SetMaterial(0, ResidenceMaterial);
	}
}
void AMainTerrain::OnMouseOutSlums(UPrimitiveComponent* Component)
{
	if (Component && SlumsMaterial)
	{
		Component->SetMaterial(0, SlumsMaterial);
	}
}

void AMainTerrain::BeginPlay()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->bShowMouseCursor = true; // Показываем курсор
		PlayerController->bEnableClickEvents = true; // Включаем обработку событий кликов
		PlayerController->bEnableMouseOverEvents = true; // Включаем обработку событий наведения
	}

	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();

	create_terrain();
	get_closed_figures(roads, figures_array, 200);
	// smooth_blocks(figures_array);
	process_blocks(figures_array);
	for (auto& b : figures_array)
	{
		process_houses(b);
	}
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

void AMainTerrain::move_river(TSharedPtr<Node>& node1, TSharedPtr<Node>& node2)
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

void AMainTerrain::move_road(TSharedPtr<Node>& node)
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

	create_guiding_rivers();
	get_river_figure();
	for (int iter = 0; iter < 100; iter++)
	{
		for (auto& b : bridges)
		{
			move_river(b.Key, b.Value);
		}
	}
	process_bridges();

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
			move_road(r);
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
			move_road(r);
		}
	}

	shrink_roads();
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
	auto start_point_left = MakeShared<Node>(FVector(0, y_size / 2 - av_river_length / 2, 0));
	auto start_point_right = MakeShared<Node>(FVector(0, y_size / 2 + av_river_length / 2, 0));
	river.Add(start_point_left);
	river.Add(start_point_right);
	// guiding_river.Add(start_point);
	add_conn(start_point_left, start_point_right);
	create_guiding_river_segment(start_point, end_point, start_point_left, start_point_right);
}

void AMainTerrain::create_guiding_river_segment(TSharedPtr<Node> start_point, TSharedPtr<Node> end_point,
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
void AMainTerrain::process_bridges()
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
		create_segment(roads, old_node, node, true, point_type::main_road, max_road_length);
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
			forward = 2;
			left = 4;
			right = 7;
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

TOptional<TSharedPtr<Node>> AMainTerrain::create_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
														 TSharedPtr<Node> end_point, bool to_exect_point,
														 point_type type, double max_length)
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
			if (tries > 10 || FVector::Distance(start_point->get_point(), end_point->get_point()) > max_length)
			{
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

void AMainTerrain::create_mesh(UProceduralMeshComponent* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
							   float ExtrusionHeight)
{
	if (!Mesh)
	{
		Mesh = NewObject<UProceduralMeshComponent>(this, TEXT("GeneratedMesh"));
		Mesh->SetupAttachment(RootComponent);
		Mesh->RegisterComponent();
	}

	int32 NumVertices = BaseVertices.Num();
	if (NumVertices < 3)
	{
		return; // Нужно хотя бы 3 вершины для создания полигона
	}

	TArray<FVector> Vertices;
	TArray<FVector> Base;
	TArray<int32> Triangles;

	// Добавляем вершины нижней стороны
	for (auto Vertex : BaseVertices)
	{
		Base.Add(Vertex + FVector(0, 0, StarterHeight));
		Vertices.Add(Vertex + FVector(0, 0, StarterHeight));
	}

	// Добавляем вершины верхней стороны, сдвинутые вверх на ExtrudeHeight
	for (auto Vertex : BaseVertices)
	{
		Vertices.Add(Vertex + FVector(0, 0, StarterHeight) + FVector(0, 0, ExtrusionHeight));
	}

	// Используем триангуляцию для нижней стороны
	TArray<int32> BaseTriangles;
	AllGeometry::TriangulatePolygon(Base, BaseTriangles);

	// Добавляем триангуляцию для нижней стороны
	for (int32 i = 0; i < BaseTriangles.Num(); i += 3)
	{
		Triangles.Add(BaseTriangles[i]);
		Triangles.Add(BaseTriangles[i + 1]);
		Triangles.Add(BaseTriangles[i + 2]);
	}

	// Добавляем триангуляцию для верхней стороны (с учетом смещения вершин)
	int32 Offset = NumVertices;
	for (int32 i = 0; i < BaseTriangles.Num(); i += 3)
	{
		Triangles.Add(BaseTriangles[i] + Offset);
		Triangles.Add(BaseTriangles[i + 2] + Offset);
		Triangles.Add(BaseTriangles[i + 1] + Offset);
	}

	// Создаем боковые стороны
	for (int32 i = 0; i < NumVertices; i++)
	{
		int32 NextIndex = (i + 1) % NumVertices;

		// Боковая сторона, первый треугольник
		Triangles.Add(NextIndex);
		Triangles.Add(i);
		Triangles.Add(NumVertices + i);

		// Боковая сторона, второй треугольник
		Triangles.Add(NumVertices + NextIndex);
		Triangles.Add(NextIndex);
		Triangles.Add(NumVertices + i);
	}

	// Создаем пустые массивы для нормалей, UV-координат и тангенсов
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Создаем меш
	Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
}

void AMainTerrain::create_mesh(UProceduralMeshComponent* MeshComponent, TArray<TSharedPtr<Node>> BaseVertices,
							   float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_point());
	}
	create_mesh(MeshComponent, vertices, StarterHeight, ExtrusionHeight);
}
void AMainTerrain::create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
							   float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->point);
	}
	create_mesh(Mesh, vertices, StarterHeight, ExtrusionHeight);
}

void AMainTerrain::get_closed_figures(TArray<TSharedPtr<Node>> lines, TArray<Block>& fig_array, int figure_threshold)
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
				FVector center_point;
				double smallest_angle = 360;
				for (int i = 0; i < second_node->conn.Num(); i++)
				{
					center_point = (second_node->conn[i]->node->get_point() + first_node->get_point()) / 2;
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
				// if (smallest_angle > 180)
				// {
				// 	second_node->set_point(center_point);
				// }
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
				fig_array.Add(Block(*figure_array));
			}
		}
	}
}
void AMainTerrain::get_river_figure()
{
	TArray<Block> river_fig_array;
	get_closed_figures(river, river_fig_array, TNumericLimits<int>::Max());
	if (river_fig_array.IsEmpty())
	{
		return;
	}
	river_fig_array.Sort([this](Block Item1, Block Item2) { return Item1.figure.Num() > Item2.figure.Num(); });

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
// void AMainTerrain::smooth_blocks(TArray<Block>& blocks)
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
void AMainTerrain::process_blocks(TArray<Block>& blocks)
{
	blocks.Sort([this](Block Item1, Block Item2) { return Item1.area > Item2.area; });

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
	int wrong_blocks = 0;
	for (auto& b : blocks)
	{
		b.get_self_figure();
		if (!b.shrink_size(b.self_figure, 4.0f))
		{
			wrong_blocks++;
		}
		b.area = AllGeometry::get_poygon_area(b.self_figure);
	}
}
void AMainTerrain::process_houses(Block& block)
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
			// auto angle2 = AllGeometry::calculate_angle(block.self_figure[i - 2].point, block.self_figure[i -
			// 1].point, 										   block.self_figure[i].point, true); auto angle1 =
			// AllGeometry::calculate_angle(block.self_figure[i - 3].point, block.self_figure[i - 2].point,
			// 										   block.self_figure[i - 1].point, true);
			// FVector point1 =
			// 	AllGeometry::create_segment_at_angle(block.self_figure[i - 2].point, block.self_figure[i - 1].point,
			// 										 block.self_figure[i - 1].point, angle1 / 2, 15);
			// FVector point2 =
			// 	AllGeometry::create_segment_at_angle(block.self_figure[i - 3].point, block.self_figure[i - 2].point,
			// 										 block.self_figure[i - 2].point, angle2 / 2, 15);
			// TArray<FVector> figure{point1, point2};
			// block.create_house(figure, 10, 10);

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

void AMainTerrain::draw_all()
{
	FlushPersistentDebugLines(GetWorld());

	// for (auto b : map_borders_array)
	// {
	// 	for (auto bconn : b->conn)
	// 	{
	// 		DrawDebugLine(GetWorld(), bconn->node->get_point(), b->get_point(), FColor::White, true, -1, 0, 20);
	// 	}
	// }
	UProceduralMeshComponent* MeshComponent =
		NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->RegisterComponent();
	MeshComponent->SetMaterial(1, BaseMaterial);
	create_mesh(MeshComponent, map_borders_array, 0, 1);

	for (auto r : river)
	{
		for (auto rconn : r->conn)
		{
			DrawDebugLine(GetWorld(), rconn->node->get_point(), r->get_point(), FColor::Blue, true, -1, 0, 1);
		}
	}

	for (int i = 0; i < roads.Num(); i++)
	{
		for (int j = 0; j < roads[i]->conn.Num(); j++)
		{
			FColor color = FColor::Green;
			DrawDebugLine(GetWorld(), roads[i]->conn[j]->node->get_point(), roads[i]->get_point(), color, true, -1, 0,
						  1);
		}
	}


	// MeshComponent2->SetMaterial(NULL, LuxuryMaterial);

	for (auto r : figures_array)
	{
		// FColor color;
		// int thickness = 1;

		TArray<TSharedPtr<Point>> figure_to_print;
		if (!r.self_figure.IsEmpty())
		{
			for (auto& p : r.self_figure)
			{
				figure_to_print.Add(MakeShared<Point>(p));
			}
		}
		else
		{
			continue;
		}

		if (r.get_type() == block_type::luxury)
		{
			UProceduralMeshComponent* MeshComponent2 =
				NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
			MeshComponent2->SetupAttachment(RootComponent);
			MeshComponent2->RegisterComponent();
			MeshComponent2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
			MeshComponent2->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
			MeshComponent2->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
			MeshComponent2->SetCollisionResponseToChannel(ECC_Visibility,
														  ECR_Block); // Разрешаем пересечение с каналом видимости

			MeshComponent2->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
			MeshComponent2->SetGenerateOverlapEvents(true); // Включаем события перекрытия
			MeshComponent2->bSelectable = true; // Делаем компонент интерактивным
			MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutLuxury);

			MeshComponent2->SetMaterial(NULL, LuxuryMaterial);
			create_mesh(MeshComponent2, figure_to_print, 1, 0.5);
		}
		else if (r.get_type() == block_type::dock)
		{
			UProceduralMeshComponent* MeshComponent2 =
				NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
			MeshComponent2->SetupAttachment(RootComponent);
			MeshComponent2->RegisterComponent();
			MeshComponent2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
			MeshComponent2->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
			MeshComponent2->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
			MeshComponent2->SetCollisionResponseToChannel(ECC_Visibility,
														  ECR_Block); // Разрешаем пересечение с каналом видимости

			MeshComponent2->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
			MeshComponent2->SetGenerateOverlapEvents(true); // Включаем события перекрытия
			MeshComponent2->bSelectable = true; // Делаем компонент интерактивным
			MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutDock);

			MeshComponent2->SetMaterial(NULL, DocsMaterial);
			create_mesh(MeshComponent2, figure_to_print, 1, 0.5);
		}
		else if (r.get_type() == block_type::royal)
		{
			UProceduralMeshComponent* MeshComponent2 =
				NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
			MeshComponent2->SetupAttachment(RootComponent);
			MeshComponent2->RegisterComponent();
			MeshComponent2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
			MeshComponent2->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
			MeshComponent2->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
			MeshComponent2->SetCollisionResponseToChannel(ECC_Visibility,
														  ECR_Block); // Разрешаем пересечение с каналом видимости

			MeshComponent2->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
			MeshComponent2->SetGenerateOverlapEvents(true); // Включаем события перекрытия
			MeshComponent2->bSelectable = true; // Делаем компонент интерактивным
			MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutRoyal);

			MeshComponent2->SetMaterial(NULL, RoyalMaterial);
			create_mesh(MeshComponent2, figure_to_print, 1, 0.5);
		}
		else if (r.get_type() == block_type::slums)
		{
			UProceduralMeshComponent* MeshComponent2 =
				NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
			MeshComponent2->SetupAttachment(RootComponent);
			MeshComponent2->RegisterComponent();
			MeshComponent2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
			MeshComponent2->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
			MeshComponent2->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
			MeshComponent2->SetCollisionResponseToChannel(ECC_Visibility,
														  ECR_Block); // Разрешаем пересечение с каналом видимости

			MeshComponent2->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
			MeshComponent2->SetGenerateOverlapEvents(true); // Включаем события перекрытия
			MeshComponent2->bSelectable = true; // Делаем компонент интерактивным
			MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutSlums);
			MeshComponent2->SetMaterial(NULL, SlumsMaterial);
			create_mesh(MeshComponent2, figure_to_print, 1, 0.5);
		}
		else if (r.get_type() == block_type::residential)
		{
			UProceduralMeshComponent* MeshComponent2 =
				NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
			MeshComponent2->SetupAttachment(RootComponent);
			MeshComponent2->RegisterComponent();
			MeshComponent2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
			MeshComponent2->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
			MeshComponent2->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
			MeshComponent2->SetCollisionResponseToChannel(ECC_Visibility,
														  ECR_Block); // Разрешаем пересечение с каналом видимости

			MeshComponent2->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
			MeshComponent2->SetGenerateOverlapEvents(true); // Включаем события перекрытия
			MeshComponent2->bSelectable = true; // Делаем компонент интерактивным
			MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutResidential);
			MeshComponent2->SetMaterial(NULL, ResidenceMaterial);
			create_mesh(MeshComponent2, figure_to_print, 1, 0.5);
		}
		for (auto& p : r.houses)
		{
			UProceduralMeshComponent* MeshComponent2 =
				NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
			MeshComponent2->SetupAttachment(RootComponent);
			MeshComponent2->RegisterComponent();
			MeshComponent2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
			MeshComponent2->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
			MeshComponent2->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
			MeshComponent2->SetCollisionResponseToChannel(ECC_Visibility,
														  ECR_Block); // Разрешаем пересечение с каналом видимости

			MeshComponent2->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
			MeshComponent2->SetGenerateOverlapEvents(true); // Включаем события перекрытия
			MeshComponent2->bSelectable = true; // Делаем компонент интерактивным
			MeshComponent2->SetMaterial(NULL, BaseMaterial);
			create_mesh(MeshComponent2, p.house_figure, 0, p.height);
		}
		// else if (r.get_type() == block_type::empty)
		// {
		// 	color = FColor(255, 255, 255);
		// 	thickness = 3;
		// }
		// DrawDebugLine(GetWorld(), figure_we_got[i - 1]->point, figure_we_got[i]->point, color, true, -1, 0,
		// 			  thickness);
	}
	{
		UProceduralMeshComponent* MeshComponent2 =
			NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
		MeshComponent2->SetupAttachment(RootComponent);
		MeshComponent2->RegisterComponent();
		MeshComponent2->SetMaterial(NULL, WaterMaterial);
		auto FirstElement = river_figure.figure[0];
		river_figure.figure.RemoveAt(0);
		river_figure.figure.Add(FirstElement);
		// int32 N = river_figure.figure.Num();
		// for (int32 i = 0; i < N / 2; i++)
		// {
		// 	river_figure.figure.Swap(i, N - i - 1);
		// }
		create_mesh(MeshComponent2, river_figure.figure, 1, 0.5);
	}
}
