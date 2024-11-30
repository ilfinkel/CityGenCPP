#pragma once

#include <City/AllGeometry.h>
#include <list>
#include <random>

#include "Algo/Reverse.h"
#include "Components/PrimitiveComponent.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* BaseMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* WaterMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* DocsMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* RoyalMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* ResidenceMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* LuxuryMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* SlumsMaterial;


	UFUNCTION()
	void OnMouseOver(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOutRoyal(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOutDock(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOutLuxury(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOutResidential(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOutSlums(UPrimitiveComponent* Component);


	// UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Procedural Mesh")
	// UProceduralMeshComponent* ProceduralMesh;
	FVector center = FVector(x_size / 2, y_size / 2, 0);
	double av_distance = (x_size + y_size) / 4;
	double av_river_length = 80;
	double max_river_length = 150;
	double min_new_road_length = 45;
	double min_road_length = 10;
	double av_road_length = 70;
	double max_road_length = 95;
	double river_road_distance = 60; //((x_size + y_size) / 2) / 20;

	UProceduralMeshComponent* BaseComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void create_mesh(UProceduralMeshComponent* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
					 float ExtrusionHeight);
	void create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight,
					 float ExtrusionHeight);
	void create_mesh(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight,
					 float ExtrusionHeight);

	void draw_all();
	void get_cursor_hit_location();
	TArray<TSharedPtr<Node>> map_borders_array;
	TArray<Block> figures_array;
	Block river_figure;
};
