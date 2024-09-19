// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <list>
#include <random>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"
#include "MainTerrain.generated.h"

enum point_type { main, main_road, road, river };

struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_)
		: point(point_), weight(weight_)
	{
	};
	FVector point;
	double weight;
};

struct Node
{
	Node(double X, double Y, double Z): node(FVector(X, Y, Z))
	{
	};

	Node(): node(FVector(0, 0, 0))
	{
	};

	Node(FVector node_): Node(node_.X, node_.Y, node_.Z)
	{
	};

	FVector node;
	point_type type;
	bool used = false;
	TArray<TSharedPtr<Node>> conn;

	friend bool operator==(const Node& Lhs, const Node& RHS)
	{
		return Lhs.node == RHS.node
			&& Lhs.type == RHS.type;
	}

	friend bool operator!=(const Node& Lhs, const Node& RHS)
	{
		return !(Lhs == RHS);
	}

	void delete_me()
	{
		for (auto c : conn)
		{
			for (int i = 0; i < c->conn.Num(); i++)
			{
				if (node == c->conn[i]->node)
				{
					c->conn.RemoveAt(i);
					break;
				}
			}
		}
	};
};


static double x_size = 5000;
static double y_size = 5000;

class AllGeometry
{
public:
	static TOptional<FVector> is_intersect(FVector line1_begin,
	                                       FVector line1_end, FVector line2_begin,
	                                       FVector line2_end, bool is_opened);


	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		TSharedPtr<Node> line1_begin,
		TSharedPtr<Node> line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static TOptional<TSharedPtr<Node>> is_intersect_array_clear(
		TSharedPtr<Node> line1_begin, TSharedPtr<Node> line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static FVector create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
	                                       const FVector& line_beginPoint,
	                                       double angle_in_degrees, double length);

	static double calculate_angle(FVector A, FVector B,
	                              FVector C);
};


UCLASS()
class CITY_API AMainTerrain : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMainTerrain();
	UPROPERTY(EditAnywhere)
	;
	double x_size_in = 2000;
	UPROPERTY(EditAnywhere)
	;
	double y_size_in = 2000;
	double av_river_length = ((x_size + y_size) / 2) / 20;
	double min_road_length = 20; //((x_size + y_size) / 2) / 100;
	double av_road_length = 45; //((x_size + y_size) / 2) / 80;
	double max_road_length = 90; //((x_size + y_size) / 2) / 60;
	double river_road_distance = 90; //((x_size + y_size) / 2) / 20;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void create_terrain();
	void draw_all();
	void tick_river(TSharedPtr<Node>& node);
	void tick_road(TSharedPtr<Node>& node);
	void create_guiding_rivers();
	void create_guiding_river_segment(TSharedPtr<Node> start_point, TSharedPtr<Node> end_point);
	// void create_guiding_river_segment(PointLine& starting_river, const TSharedPtr<RiverNode>& start_point);
	void create_guiding_roads();
	void create_usual_roads();
	void create_usual_road_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
	                               TSharedPtr<Node> end_point);
	bool create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point);
	void shrink_roads();
	void point_shift(FVector& node);
	TArray<TSharedPtr<Node>> river;
	TArray<TSharedPtr<Node>> roads;
	TArray<TSharedPtr<Node>> road_centers;
	TArray<FVector> map_points_array;
	TArray<TSharedPtr<Node>> map_borders_array;
	TArray<TSharedPtr<Node>> guididng_roads_array;
	TArray<WeightedPoint> weighted_points;
};
