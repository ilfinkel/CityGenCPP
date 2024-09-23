// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <list>
#include <random>
#include<City/AllGeometry.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"
#include "ProceduralMeshComponent.h"
#include "MainTerrain.generated.h"

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


	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Procedural Mesh")
	UProceduralMeshComponent* ProceduralMesh;

	double y_size_in = 2000;
	double av_river_length = ((x_size + y_size) / 2) / 20;
	double min_road_length = ((x_size + y_size) / 2) / 100;
	double av_road_length = ((x_size + y_size) / 2) / 80;
	double max_road_length = ((x_size + y_size) / 2) / 60;
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
	void create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float ExtrusionHeight);
	void shrink_roads();
	void point_shift(FVector& node);
	void get_closed_figures(TArray<TSharedPtr<Node>> lines);
	//void get_figure(TArray<TSharedPtr<Node>>& lines, TArray<TSharedPtr<Node>>& node_array, TSharedPtr<Node> node1, TSharedPtr<Node> node2);
	//void get_into_figure(TArray<TSharedPtr<Node>> lines, TArray<TSharedPtr<Node>>& node_array);
	TArray<TSharedPtr<Node>> river;
	TArray<TSharedPtr<Node>> roads;
	TArray<TSharedPtr<Node>> road_centers;
	TArray<FVector> map_points_array;
	TArray<TSharedPtr<Node>> map_borders_array;
	TArray<TSharedPtr<Node>> guididng_roads_array;
	TArray<WeightedPoint> weighted_points;
	TArray<TArray<TSharedPtr<Node>>> figures_array;
};
