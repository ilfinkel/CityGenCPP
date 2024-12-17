#pragma once

#include <City/AllGeometry.h>

#include ".git/ProceduralObjectMeshActor.h"
#include "Algo/Reverse.h"
#include "Components/PrimitiveComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"

#include "MainTerrain.generated.h"

UENUM(BlueprintType)
enum class ECityPlan : uint8
{
	radial UMETA(DisplayName = "radial"),
	radial_circle UMETA(DisplayName = "radial-circle"),
	rectangular UMETA(DisplayName = "rectangular"),
	combined UMETA(DisplayName = "combined")

};

USTRUCT(BlueprintType)
struct FMapParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double av_river_length = 80;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double max_river_length = 150;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double min_new_road_length = 45;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double min_road_length = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double av_road_length = 70;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double max_road_length = 95;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double river_road_distance = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityPlan")
	ECityPlan city_plan = ECityPlan::combined;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double x_size = 4000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double y_size = 4000;
	FVector center = FVector(x_size / 2, y_size / 2, 0);
	double av_distance = (x_size + y_size) / 4;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMapParams MapParams;
	//
	// UFUNCTION(BlueprintCallable, Category = "CustomParams")
	// void MyFunctionWithStruct(const FMyCustomParams& Params) {};
	// UFUNCTION()
	// void OnMouseOver(UPrimitiveComponent* Component);
	// UFUNCTION()
	// void OnMouseOutRoyal(UPrimitiveComponent* Component);
	// UFUNCTION()
	// void OnMouseOutDock(UPrimitiveComponent* Component);
	// UFUNCTION()
	// void OnMouseOutLuxury(UPrimitiveComponent* Component);
	// UFUNCTION()
	// void OnMouseOutResidential(UPrimitiveComponent* Component);
	// UFUNCTION()
	// void OnMouseOutSlums(UPrimitiveComponent* Component);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> VerticesRemembered;

	// double x_size = MapParams.x_size;
	// double y_size = MapParams.y_size;
	// FVector center = FVector(x_size / 2, y_size / 2, 0);
	// double av_distance = (x_size + y_size) / 4;
	// double av_river_length = MapParams.av_river_length;
	// double max_river_length = MapParams.max_river_length;
	// double min_new_road_length = MapParams.min_new_road_length;
	// double min_road_length = MapParams.min_road_length;
	// double av_road_length = MapParams.av_road_length;
	// double max_road_length = MapParams.max_road_length;
	// double river_road_distance = MapParams.river_road_distance;

	UProceduralMeshComponent* BaseComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
						float ExtrusionHeight);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight,
						float ExtrusionHeight);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight,
						float ExtrusionHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight);
	void draw_all_3d();
	void draw_all_2d();
	void get_cursor_hit_location();
	TArray<TSharedPtr<Node>> map_borders_array;
	TArray<District> figures_array;
	TArray<FVector> debug_points_array;
	TArray<TSharedPtr<Node>> roads;
	District river_figure;
};
