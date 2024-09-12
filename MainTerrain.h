// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <list>
#include <random>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"
#include "MainTerrain.generated.h"

enum point_type { main, road, river };

struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_)
		: point(point_), weight(weight_)
	{
	};
	FVector point;
	double weight;
};

struct PointLine
{
	PointLine(): line_begin(FVector(0, 0, 0)), line_end(FVector(0, 0, 0))
	{
	};

	PointLine(const FVector& line_begin_, const FVector& line_end_)
		: line_begin(line_begin_), line_end(line_end_)
	{
	};

	PointLine(const FVector& line_begin_, const FVector& line_end_,
	          point_type type_)
		: line_begin(line_begin_), line_end(line_end_), type(type_)
	{
	};

	FVector line_begin;
	FVector line_end;


	bool operator==(const PointLine& other_line_)
	{
		return (this->line_begin == other_line_.line_end &&
				this->line_end == other_line_.line_begin) ||
			(this->line_begin == other_line_.line_begin &&
				this->line_end == other_line_.line_end);
	};

	double length() const { return FVector::Dist(line_begin, line_end); };
	point_type type;
};

struct Node
{
	Node(double X, double Y, double Z): node(FVector(X, Y, Z))
	{
	};

	Node(): node(FVector(0, 0, 0))
	{
	};

	Node(FVector node_): node(FVector{node_.X, node_.Y, node_.Z})
	{
	};
	// ~Node(){
	// 	if(conn.Num() > 0)
	// 	{
	// 		for(auto c:conn)
	// 		{
	// 			for(int i = 0; i < c->conn.Num(); i++)
	// 			{
	// 				if(c->conn[i]->node==node)
	// 				{
	// 					c->conn[i] = nullptr;
	// 					c->conn.RemoveAt(i);
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	FVector node;
	bool used = false;
	TArray<TSharedPtr<Node>> conn;
};

struct RiverNode
{
	RiverNode(): node(FVector(0, 0, 0))
	{
	};

	RiverNode(FVector node_): node(FVector{node_.X, node_.Y, node_.Z})
	{
	};
	TArray<TSharedPtr<RiverNode>> prev;
	TArray<TSharedPtr<RiverNode>> next;
	FVector node;
};


struct RiverLine : PointLine
{
	RiverLine(const FVector& line_begin_, const FVector& line_end_)
		: PointLine(line_begin_, line_end_)
	{
		type = river;
	};

	RiverLine(const FVector& line_begin_, const FVector& line_end_,
	          point_type type_)
		: PointLine(line_begin_, line_end_, type_)
	{
		type = river;
	};
	FVector line_begin;
	FVector line_end;
};

static double x_size = 5000;
static double y_size = 5000;

class AllGeometry
{
public:
	static void create_main_line(PointLine line);
	static TOptional<FVector> is_intersect(const PointLine& line1,
	                                       const PointLine& line2,
	                                       bool is_opened);

	static TOptional<FVector> is_intersect(FVector line1_begin,
	                                       FVector line1_end, FVector line2_begin,
	                                       FVector line2_end, bool is_opened);

	static TOptional<TTuple<FVector, PointLine>> is_intersect_array(
		PointLine line1, TArray<PointLine> lines, bool is_opened);


	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		TSharedPtr<Node> line1_begin,
		TSharedPtr<Node> line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static TOptional<TTuple<FVector, TTuple<TSharedPtr<RiverNode>, TSharedPtr<RiverNode>>>> is_intersect_array(
		TSharedPtr<Node> line1_begin,
		TSharedPtr<Node> line1_end, const TArray<TSharedPtr<RiverNode>> lines, bool is_opened);
	static TOptional<TSharedPtr<Node>> is_intersect_array_clear(
		TSharedPtr<Node> line1_begin, TSharedPtr<Node> line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static TOptional<TSharedPtr<RiverNode>> is_intersect_array_clear(
		TSharedPtr<Node> line1_begin, TSharedPtr<Node> line1_end, const TArray<TSharedPtr<RiverNode>> lines,
		bool is_opened);
	static PointLine create_segment_at_angle(const PointLine& BaseSegment,
	                                         const FVector& line_beginPoint,
	                                         double angle_in_degrees, double length,
	                                         point_type p_type);
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
	double av_road_length = ((x_size + y_size) / 2) / 40;
	double min_road_length = 50; //((x_size + y_size) / 2) / 50;
	double max_road_length = 70; //((x_size + y_size) / 2) / 30;
	double river_road_distance = 90; //((x_size + y_size) / 2) / 20;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void create_terrain();
	void draw_all();
	void tick_terrain();
	void tick_river(TSharedPtr<RiverNode>& node);
	void tick_road(TSharedPtr<Node>& node);
	void create_guiding_rivers();
	void create_guiding_roads();
	void create_usual_roads();
	void create_usual_road_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
	                               TSharedPtr<Node> end_point);
	void create_guiding_river_segment(PointLine& starting_river, const TSharedPtr<RiverNode>& start_point);
	void create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point);
	void point_shift(FVector& node);
	TArray<TSharedPtr<RiverNode>> river;
	TArray<TSharedPtr<Node>> roads;
	TArray<TSharedPtr<Node>> road_centers;
	TArray<FVector> map_points_array;
	TArray<PointLine> map_lines_array;
	TArray<PointLine> main_lines_array;
	TArray<TSharedPtr<Node>> guididng_roads_array;
	TArray<WeightedPoint> weighted_points;
};
