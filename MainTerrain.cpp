#include "MainTerrain.h"

//#include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() { PrimaryActorTick.bCanEverTick = true; }

void AMainTerrain::BeginPlay()
{
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();

	create_terrain();
	//get_closed_figures(roads);
	draw_all();
}

// Called every frame
//void AMainTerrain::Tick(float DeltaTime)
//{
// Super::Tick(DeltaTime);
//// create_usual_roads();
// draw_all();
//}


void AMainTerrain::tick_river(TSharedPtr<Node>& node)
{
	point_shift(node->get_node()->point);

	if ((node->get_node()->point.X) < 0)
	{
		node->get_node()->point.X = 0;
	}
	if ((node->get_node()->point.Y) < 0)
	{
		node->get_node()->point.Y = 0;
	}
	if ((node->get_node()->point.X) > x_size)
	{
		node->get_node()->point.X = x_size;
	}
	if ((node->get_node()->point.Y) > y_size)
	{
		node->get_node()->point.Y = y_size;
	}
	FVector all_point(0, 0, 0);
	double count = 0;
	for (int i = 0; i < node->conn.Num(); i++)
	{
		all_point += node->conn[i].node->get_point();
		count++;
	}
	bool is_break = false;
	for (int i = 0; i < node->conn.Num(); i++)
	{
		if (is_break)
		{
			break;
		}

		if (FVector::Distance(node->get_point(), node->conn[i].node->get_point()) > (y_size / 15))
		{
			node->get_node()->point = all_point / count;
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
	point_shift(node->get_node()->point);

	if (AllGeometry::is_intersect_array(node, backup_node, river, true).IsSet())
	{
		node = backup_node;
		return;
	}

	if ((node->get_node()->point.X) < 0)
	{
		node->get_node()->point.X = 0;
	}
	if ((node->get_node()->point.Y) < 0)
	{
		node->get_node()->point.Y = 0;
	}
	if ((node->get_node()->point.X) > x_size)
	{
		node->get_node()->point.X = x_size;
	}
	if ((node->get_node()->point.Y) > y_size)
	{
		node->get_node()->point.Y = y_size;
	}
	if (!node->conn.IsEmpty())
	{
		FVector middle_point(0, 0, 0);
		for (auto p : node->conn)
		{
			middle_point += p.node->get_point();
		}
		middle_point /= node->conn.Num();

		bool is_continuing = (node->conn.Num() == 2);
		for (int i = 0; i < node->conn.Num(); i++)
		{
			for (int j = 1; j < node->conn.Num(); j++)
			{
				if (i == j) { continue; }
				if (FVector::Distance(node->get_point(), node->conn[i].node->get_point()) > (y_size / 30) || (
					is_continuing && AllGeometry::calculate_angle(node->conn[0].node->get_point(), node->get_point(),
					                                              node->conn[1].node->get_point()) < 155.0))
				{
					node->node = middle_point;
					return;
				}
			}
		}
	}
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
	double points_count = 64;
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
				(rand() % static_cast<int>(av_size / point_row_counter / 2)) + (av_size / point_row_counter / 3)
			});
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
	int old_nodes = 0;
	while (roads.Num() != old_nodes)
	{
		old_nodes = roads.Num();
		create_usual_roads();
	}
	shrink_roads();
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
	//auto test_node_node1 = MakeShared<Node>(300, 300, 0);
	//auto test_node_node2 = MakeShared<Node>(300, y_size-300, 0);
	//auto test_node_node3 = MakeShared<Node>(x_size-300, y_size-300, 0);
	////test_node_node4 = Node(x_size-300, 300, 0);
	//test_node_node1->conn.Add(test_node_node2);
	//test_node_node1->conn.Add(test_node_node3);
	//test_node_node2->conn.Add(test_node_node1);
	//test_node_node2->conn.Add(test_node_node3);
	//test_node_node3->conn.Add(test_node_node2);
	//test_node_node3->conn.Add(test_node_node1);
	////test_node_node4.conn.Add(MakeShared<Node>(test_node_node3));/*
	////test_node_node4.conn.Add(MakeShared<Node>(test_node_node1));*/
	//roads.Add(test_node_node1);
	//roads.Add(test_node_node2);
	//roads.Add(test_node_node3);
	////roads.Add(MakeShared<Node>(test_node_node4));
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
	// placing bridges
	for (auto r : river)
	{
		if (FVector::Dist(r->node, FVector(x_size / 2, y_size / 2, 0)) < (y_size / 3) && rand() % 8 >= 6 && r->conn.
			Num() == 2)
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
		int success_roads = 0;
		for (int j = 0; j < local_road.Num() - 1 && success_roads < 4; j++)
		{
			success_roads += create_guiding_road_segment(roads[i + 1], local_road[j]);
		}
	}
}

bool AMainTerrain::create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point)
{
	if (AllGeometry::is_intersect_array(start_point, end_point, river, false).
		IsSet())
	{
		return false;
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
	roads.Sort([this](auto Item1, auto Item2)
	{
		return FMath::FRand() < 0.5f;
	});
	TArray<TSharedPtr<Node>> add_road;
	int forward;
	int left;
	int right;

	for (auto r : roads)
	{
		if (!r->used)
		{
			//if (r->type == point_type::main_road)
			//{
			//	forward = 6;
			//	left = 10;
			//	right = 12;
			//}
			//else
			//{
			forward = 3;
			left = 9;
			right = 11;
			//}
			if (r->conn.Num() == 1)
			{
				if (rand() % 16 >= forward)
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
						if (FVector::Distance(rc->node, line1) < rc->conn.Num() * (y_size / 28))
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
				if (rand() % 16 >= left)
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
						if (FVector::Distance(rc->node, line2) < rc->conn.Num() * (y_size / 28))
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
				if (rand() % 16 >= right)
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
						if (FVector::Distance(rc->node, line3) < rc->conn.Num() * (y_size / 28))
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
	for (int i = 0; i < roads.Num(); i++)
	{
		if (!roads[i]->used)
		{
			for (int count = 0; count < 3; count++)
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

void AMainTerrain::create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
                               float ExtrusionHeight)
{
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = ProceduralMesh;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	// Шаг 1: Добавляем вершины базовой фигуры и экструзированные вершины
	int32 NumBaseVertices = BaseVertices.Num();

	// Добавляем вершины плоской фигуры (нижняя плоскость)
	for (auto Vertex : BaseVertices)
	{
		Vertices.Add(Vertex->node); // Нижняя грань
	}

	// Добавляем экструзированные вершины (верхняя плоскость)
	for (auto Vertex : BaseVertices)
	{
		Vertices.Add(Vertex->node + FVector(0, 0, ExtrusionHeight)); // Верхняя грань
	}

	// Шаг 2: Создаем треугольники для боковых граней
	for (int32 i = 0; i < NumBaseVertices; i++)
	{
		int32 NextIndex = (i + 1) % NumBaseVertices; // Индекс следующей вершины для замыкания

		// Боковая грань (состоит из двух треугольников)
		Triangles.Add(i); // Нижняя грань (текущая вершина)
		Triangles.Add(i + NumBaseVertices); // Верхняя грань (текущая экструзированная вершина)
		Triangles.Add(NextIndex); // Нижняя грань (следующая вершина)

		Triangles.Add(NextIndex); // Нижняя грань (следующая вершина)
		Triangles.Add(i + NumBaseVertices); // Верхняя грань (текущая экструзированная вершина)
		Triangles.Add(NextIndex + NumBaseVertices); // Верхняя грань (следующая экструзированная вершина)
	}

	// Шаг 3: Создаем треугольники для нижней грани
	for (int32 i = 1; i < NumBaseVertices - 1; i++)
	{
		Triangles.Add(0);
		Triangles.Add(i);
		Triangles.Add(i + 1);
	}

	// Шаг 4: Создаем треугольники для верхней грани (перевёрнутые индексы, чтобы нормали смотрели вверх)
	for (int32 i = 1; i < NumBaseVertices - 1; i++)
	{
		Triangles.Add(NumBaseVertices); // Начальная точка верхней грани
		Triangles.Add(NumBaseVertices + i + 1);
		Triangles.Add(NumBaseVertices + i);
	}

	// Шаг 5: Создаем нормали, UV и другие параметры (опционально)
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Добавляем заготовки для нормалей, UV и т.д.
	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		Normals.Add(FVector(0, 0, 1)); // Пример, что все нормали смотрят вверх
		UVs.Add(FVector2D(0, 0)); // Заглушка UV
		VertexColors.Add(FLinearColor::White); // Цвет вершин (белый)
		Tangents.Add(FProcMeshTangent(1, 0, 0)); // Пример тангенса
	}

	// Шаг 6: Создаем Mesh секцию
	Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}


void AMainTerrain::get_closed_figures(TArray<TSharedPtr<Node>> lines)
{
	//for (auto l : lines) {
	//	for (auto conn : l->conn) {
	//		FVector center_point = (l->node + conn->node) / 2;
	//		double dist = FVector::Dist(l->node, center_point);
	//		FVector in_fig_point = AllGeometry::create_segment_at_angle(l->node, conn->node, center_point, 90, 1);
	//		FVector in_fig_point1 = AllGeometry::create_segment_at_angle(l->node, conn->node, in_fig_point, 0, dist+10);
	//		FVector in_fig_point2 = AllGeometry::create_segment_at_angle(l->node, conn->node, in_fig_point, 180, dist+10);
	//		auto in_fig_point1_fin = AllGeometry::is_intersect_array(in_fig_point, in_fig_point1, roads, true);
	//		in_fig_point1 = (in_fig_point1_fin.IsSet() ? in_fig_point1_fin.GetValue().Key : in_fig_point1);
	//		auto in_fig_point2_fin = AllGeometry::is_intersect_array(in_fig_point, in_fig_point2, roads, true);
	//		in_fig_point2 = (in_fig_point2_fin.IsSet() ? in_fig_point2_fin.GetValue().Key : in_fig_point2);
	//		//if (in_fig_point1 == in_fig_point2) continue;
	//		if (FVector::Dist(in_fig_point1, in_fig_point2) > max_road_length+5) continue;
	//		if (FVector::Dist(in_fig_point1, in_fig_point2) < min_road_length-5) continue;
	//		FigLine fig;
	//		TArray<TSharedPtr<Node>> fig_figure;
	//		fig_figure.Add(l);
	//		fig_figure.Add(conn);
	//		fig.figure = MakeShared<TArray<TSharedPtr<Node>>>(fig_figure);

	//		fig.node_point1 = in_fig_point1;
	//		fig.node_point2 = in_fig_point2;

	//		for (auto& fig_line : fig_lines_array) {
	//			auto intersection1 = AllGeometry::is_intersect(in_fig_point, in_fig_point1, fig_line.node_point1, fig_line.node_point2, false);
	//			auto intersection2 = AllGeometry::is_intersect(in_fig_point, in_fig_point2, fig_line.node_point1, fig_line.node_point2, false);
	//			if (intersection1.IsSet()) {
	//				fig.figure = fig_line.figure;
	//				fig_line.figure->Add(l);
	//			}
	//			if (intersection2.IsSet()) {
	//				fig.figure = fig_line.figure;
	//				fig_line.figure->Insert(conn,0);
	//			}
	//			if(intersection1.IsSet() || intersection2.IsSet()){ break; }
	//			
	//		}
	//		fig_lines_array.Add(fig);
	//	}
	//}
	//for (auto fig : fig_lines_array) {
	//	if(fig.figure.IsValid()){ figures_array.AddUnique(*fig.figure); }
	//}
}

//void AMainTerrain::get_figure(TArray<TSharedPtr<Node>>& lines, TArray<TSharedPtr<Node>>& current_figure, TSharedPtr<Node> node1, TSharedPtr<Node> node2) {
//	for (auto l : current_figure) {
//		UE_LOG(LogTemp, Warning, TEXT("CurrentFigurePoints: %f,%f"), l->node.X, l->node.Y);
//	}
//	
//	UE_LOG(LogTemp, Warning, TEXT("Points: %f,%f,%f,%f "), node1->node.X, node1->node.Y, node2->node.X, node2->node.Y);
//	UE_LOG(LogTemp, Warning, TEXT("node2->conn.Num() = %d"), node2->conn.Num());
//	for (auto& cnode : node2->conn)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//		if (node1->node != cnode->node)
//		{
//			UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//			//if (cnode->used == true) continue;
//			//TArray<TSharedPtr<Node>> figure_array;
//			bool is_figure = false;
//			for (auto& n : current_figure) {
//				UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//
//				UE_LOG(LogTemp, Warning, TEXT("n->node == cnode->node: %f,%f,%f,%f "), n->node.X, n->node.Y, cnode->node.X, cnode->node.Y);
//				if (n->node == cnode->node) {
//					UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//					is_figure = true;
//				}
//				if (is_figure) {
//					UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//					if (current_figure.Num() > 0) { 
//						auto closing_node = current_figure[0];
//						current_figure.Add(closing_node);
//						figures_array.Add(current_figure);
//					}
//					break;
//				}
//				else {
//					UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//					current_figure.Add(cnode);
//					get_figure(lines, current_figure, node2, cnode);
//					break;
//				}
//			}
//			if (is_figure) {
//				UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//				//get_into_figure(lines, figure_array);
//				break;
//			}
//		}
//	}
//}
//
//void AMainTerrain::get_into_figure(TArray<TSharedPtr<Node>> lines, TArray<TSharedPtr<Node>>& node_array)
//{
//	TArray<TSharedPtr<Node>> lines1 = node_array;
//	TArray<TSharedPtr<Node>> lines2 = node_array;
//	int lines1_count = lines1.Num();
//	int lines2_count = lines2.Num();
//	for (auto l : lines) {
//		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//		FVector test_line_end(l->node.X, y_size, l->node.Z);
//		if (AllGeometry::is_intersect_array_counter(l->node, test_line_end, node_array, false) % 2 == 1) {
//			lines1.Add(l);
//		}
//		else {
//			lines2.Add(l);
//		}
//	}
//	if (lines1.Num() == lines1_count) {
//		figures_array.Add(lines1);
//		for (auto point : lines1) {
//			UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//			int counter = 0;
//			for (auto figure : figures_array) {
//				UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//				for (auto figure_point : figure) {
//					UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//					if (point->node == figure_point->node) {
//						counter++;
//					}
//				}
//			}
//			if (point->conn.Num() == counter) {
//				point->used = true;
//			}
//		}
//	}
//	if (lines2.Num() == lines2_count) {
//		figures_array.Add(lines2);
//		for (auto point : lines2) {
//			UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//			int counter = 0;
//			for (auto figure : figures_array) {
//				UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//				for (auto figure_point : figure) {
//					UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
//					if (point->node == figure_point->node) {
//						counter++;
//					}
//				}
//			}
//			if (point->conn.Num() == counter) {
//				point->used = true;
//			}
//		}
//	}
//	get_closed_figures(lines1);
//	get_closed_figures(lines2);
//
//}

void AMainTerrain::draw_all()
{
	FlushPersistentDebugLines(GetWorld());

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
			else
			{
				DrawDebugLine(GetWorld(), rconn->node, r->node, FColor::Green, true,
				              -1, 0, 5);
			}
		}
	}

	//for (auto fig : fig_lines_array) {
	//	//fig.Add(fig[0]);
	//	//for (int i = 1; i < fig.Num(); i++) {
	//	//	//if (fig[i-1]->type == point_type::main_road && fig[i]->type == point_type::main_road)
	//	//	//{
	//	//	//	DrawDebugLine(GetWorld(), fig[i - 1]->node, fig[i]->node, FColor::Red, true, -1, 0, 9);
	//	//	//}
	//	//	//else
	//	//	//{
	//	//	fig[i - 1]->node.Z = 20;
	//	//	fig[i]->node.Z = 20;
	//	//		DrawDebugLine(GetWorld(), fig[i - 1]->node, fig[i]->node, FColor::Red, true, -1, 0, 5);
	//	//	//}
	//	//}
	//	FVector p1 = fig.node_point1;
	//	p1.Z = 20;
	//	FVector p2 = fig.node_point2;
	//	p2.Z = 20;
	//	DrawDebugLine(GetWorld(), p1, p2, FColor::Red, true, -1, 0, 5); ;
	//}
}

//TOptional<FVector> AllGeometry::is_intersect(FVector line1_begin,
//                                             FVector line1_end,
//                                             FVector line2_begin,
//                                             FVector line2_end, bool is_opened)
//{
//	double dx1 = line1_end.X - line1_begin.X;
//	double dy1 = line1_end.Y - line1_begin.Y;
//	double dx2 = line2_end.X - line2_begin.X;
//	double dy2 = line2_end.Y - line2_begin.Y;
//
//	double det = dx1 * dy2 - dx2 * dy1;
//
//	if (std::abs(det) < 1e-6)
//	{
//		return TOptional<FVector>();
//	}
//
//	double t1 = ((line2_begin.X - line1_begin.X) * dy2 -
//			(line2_begin.Y - line1_begin.Y) * dx2) /
//		det;
//	double t2 = ((line2_begin.X - line1_begin.X) * dy1 -
//			(line2_begin.Y - line1_begin.Y) * dx1) /
//		det;
//
//	if (t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0)
//	{
//		FVector intersectionPoint(line1_begin.X + t1 * dx1,
//		                          line1_begin.Y + t1 * dy1, 0);
//		if (is_opened)
//		{
//			UE_LOG(LogTemp, Warning, TEXT("intersected!"));
//			UE_LOG(LogTemp, Warning, TEXT("Point: %f,%f "), intersectionPoint.X,
//			       intersectionPoint.Y);
//			return intersectionPoint;
//		}
//		if (!is_opened && (FVector::Distance(intersectionPoint, line2_begin) >
//			TNumericLimits<double>::Min() &&
//			FVector::Distance(intersectionPoint, line2_end) >
//			TNumericLimits<double>::Min() &&
//			FVector::Distance(intersectionPoint, line1_begin) >
//			TNumericLimits<double>::Min() &&
//			FVector::Distance(intersectionPoint, line1_end) >
//			TNumericLimits<double>::Min()))
//		{
//			return intersectionPoint;
//		}
//	}
//
//	return TOptional<FVector>();
//}
//
//
//TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>
//AllGeometry::is_intersect_array(TSharedPtr<Node> line1_begin,
//                                TSharedPtr<Node> line1_end,
//                                const TArray<TSharedPtr<Node>> lines,
//                                bool is_opened)
//{
//	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
//	double dist = TNumericLimits<double>::Max();
//	FVector intersect_point_final(0, 0, 0);
//	for (auto& line : lines)
//	{
//		for (auto& conn : line->conn)
//		{
//			TOptional<FVector> int_point = is_intersect(
//				line1_begin->node, line1_end->node, line->node, conn->node, is_opened);
//			if (int_point.IsSet())
//			{
//				double dist_to_line = FVector::Dist(line1_begin->node, int_point.GetValue());
//				if (dist_to_line < dist)
//				{
//					// point_line = PointLine(line->node, conn->node);
//					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{line, conn};
//
//					dist = dist_to_line;
//					intersect_point_final = int_point.GetValue();
//				}
//			}
//		}
//	}
//	if (dist == TNumericLimits<double>::Max())
//	{
//		return TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
//	}
//	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{intersect_point_final, point_line};
//	return final_tuple;
//}
//
//int AllGeometry::is_intersect_array_counter(FVector line1_begin, FVector line1_end,
//	const TArray<TSharedPtr<Node>> lines, bool is_opened)
//{
//	int counter = 0;
//	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
//	double dist = TNumericLimits<double>::Max();
//	for (auto& line : lines)
//	{
//		for (auto& conn : line->conn)
//		{
//			TOptional<FVector> int_point = is_intersect(
//				line1_begin, line1_end, line->node, conn->node, is_opened);
//			if (int_point.IsSet())
//			{
//				counter++;
//			}
//		}
//	}
//	return counter/2;
//}
//
//TOptional<TSharedPtr<Node>> AllGeometry::is_intersect_array_clear(TSharedPtr<Node> line1_begin,
//                                                                  TSharedPtr<Node> line1_end,
//                                                                  const TArray<TSharedPtr<Node>> lines, bool is_opened)
//{
//	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
//	if (!inter_segment.IsSet())
//	{
//		return TOptional<TSharedPtr<Node>>();
//	}
//	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->node) < FVector::Dist(
//		       inter_segment->Key, inter_segment->Value.Value->node)
//		       ? inter_segment->Value.Key
//		       : inter_segment->Value.Value;
//}
//
//FVector AllGeometry::create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
//                                             const FVector& line_beginPoint, double angle_in_degrees, double length)
//{
//	double Dx = line_end.X - line_begin.X;
//	double Dy = line_end.Y - line_begin.Y;
//
//	double AngleInRadians = FMath::DegreesToRadians(angle_in_degrees);
//
//	double NewX = line_beginPoint.X + (Dx * FMath::Cos(AngleInRadians) - Dy * FMath::Sin(AngleInRadians));
//	double NewY = line_beginPoint.Y + (Dx * FMath::Sin(AngleInRadians) + Dy * FMath::Cos(AngleInRadians));
//
//	FVector line_endPointBL{NewX, NewY, line_end.Z};
//
//	auto seg_length = FVector::Dist(line_beginPoint, line_endPointBL);
//	Dx = line_endPointBL.X - line_beginPoint.X;
//	Dy = line_endPointBL.Y - line_beginPoint.Y;
//
//	NewX = line_beginPoint.X + Dx * length / seg_length;
//	NewY = line_beginPoint.Y + Dy * length / seg_length;
//
//	FVector line_endPoint(NewX, NewY, line_end.Z);
//
//	return line_endPoint;
//}
//
//double AllGeometry::calculate_angle(const FVector A, const FVector B,
//                                    const FVector C)
//{
//	FVector BA = A - B;
//	FVector BC = C - B;
//	BA.Normalize();
//	BC.Normalize();
//	double CosTheta = FVector::DotProduct(BA, BC);
//	CosTheta = FMath::Clamp(CosTheta, -1.0f, 1.0f);
//	double AngleRadians = FMath::Acos(CosTheta);
//	double AngleDegrees = FMath::RadiansToDegrees(AngleRadians);
//	return AngleDegrees;
//}
