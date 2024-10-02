#pragma once

#include <City/AllGeometry.h>
#include <list>
#include <random>

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

	// UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Procedural Mesh")
	// UProceduralMeshComponent* ProceduralMesh;
	FVector center = FVector(x_size / 2, y_size / 2, 0);
	double av_river_length = ((x_size + y_size) / 2) / 20;
	double min_road_length = 45;
	double av_road_length = 70;
	double max_road_length = 95;
	double river_road_distance = 60; //((x_size + y_size) / 2) / 20;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void add_conn(TSharedPtr<Node> node1, TSharedPtr<Node> node2);
	TSharedPtr<Node> insert_conn(TSharedPtr<Node> node1_to_insert, TSharedPtr<Node> node2_to_insert,
								 FVector node3_point);
	void create_terrain();
	void tick_river(TSharedPtr<Node>& node);
	void tick_road(TSharedPtr<Node>& node);
	void create_guiding_rivers();
	void create_guiding_river_segment(TSharedPtr<Node> start_point, TSharedPtr<Node> end_point);
	void create_guiding_roads();
	void create_usual_roads();
	void fix_broken_roads();
	TOptional<TSharedPtr<Node>> create_road_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
													TSharedPtr<Node> end_point, bool to_exect_point, point_type type);
	bool create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point);
	void create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float ExtrusionHeight);
	void shrink_roads();
	void point_shift(FVector& point);
	void smooth_road_corners();
	void get_closed_figures(TArray<TSharedPtr<Node>> lines);
	void process_blocks(TArray<Block>& blocks);
	void draw_all();
	TArray<TSharedPtr<Node>> river;
	TArray<TSharedPtr<Node>> roads;
	TArray<TSharedPtr<Node>> road_centers;
	TArray<FVector> map_points_array;
	TArray<TSharedPtr<Node>> map_borders_array;
	TArray<TSharedPtr<Node>> guididng_roads_array;
	TArray<WeightedPoint> weighted_points;
	TArray<Block> figures_array;
};
