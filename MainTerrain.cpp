#include "MainTerrain.h"
#include ".git/ProceduralObjectMeshActor.h"
#include "TerrainGen.h"


// #include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() :
	BaseMaterial(nullptr), WaterMaterial(nullptr), DocsMaterial(nullptr), RoyalMaterial(nullptr),
	ResidenceMaterial(nullptr), LuxuryMaterial(nullptr), SlumsMaterial(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}
// void AMainTerrain::OnMouseOver(UPrimitiveComponent* Component)
// {
// 	if (BaseMaterial)
// 	{
// 		Component->SetMaterial(0, BaseMaterial);
// 	}
// }
// void AMainTerrain::OnMouseOutRoyal(UPrimitiveComponent* Component)
// {
// 	if (Component && RoyalMaterial)
// 	{
// 		Component->SetMaterial(0, RoyalMaterial);
// 	}
// }
// void AMainTerrain::OnMouseOutDock(UPrimitiveComponent* Component)
// {
// 	if (Component && DocsMaterial)
// 	{
// 		Component->SetMaterial(0, DocsMaterial);
// 	}
// }
// void AMainTerrain::OnMouseOutLuxury(UPrimitiveComponent* Component)
// {
// 	if (Component && LuxuryMaterial)
// 	{
// 		Component->SetMaterial(0, LuxuryMaterial);
// 	}
// }
// void AMainTerrain::OnMouseOutResidential(UPrimitiveComponent* Component)
// {
// 	if (Component && ResidenceMaterial)
// 	{
// 		Component->SetMaterial(0, ResidenceMaterial);
// 	}
// }
// void AMainTerrain::OnMouseOutSlums(UPrimitiveComponent* Component)
// {
// 	if (Component && SlumsMaterial)
// 	{
// 		Component->SetMaterial(0, SlumsMaterial);
// 	}
// }

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


	TerrainGen gen(center, av_distance, av_river_length, max_river_length, min_new_road_length, min_road_length,
				   av_road_length, max_road_length, river_road_distance);
	gen.create_terrain(roads, figures_array, river_figure, map_borders_array, debug_points_array);
	draw_all_2d();
	AActor* ViewTarget = PlayerController->GetViewTarget();
	if (ViewTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"), *ViewTarget->GetName());
	}

	// FVector CameraLocation = FVector(0, 0, av_distance);
	// ViewTarget->SetActorLocation(CameraLocation);
	//
	// FRotator CameraRotation = FRotator(-90.0f, 0.0f, 0.0f);
	// ViewTarget->SetActorRotation(CameraRotation);
	//
	//
	// // AActor* ViewTarget = PlayerController->GetViewTarget();
	//
	// if (PlayerController && ViewTarget)
	// {
	// 	PlayerController->SetViewTarget(ViewTarget);
	// }
}

// Called every frame
void AMainTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	get_cursor_hit_location();

	//// create_usual_roads();
	// draw_all();
}

void AMainTerrain::create_mesh_3d(UProceduralMeshComponent* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
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
		auto local_vertex = Mesh->GetComponentTransform().InverseTransformPosition(Vertex);
		Base.Add(local_vertex + FVector(0, 0, StarterHeight));
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight));
	}

	// Добавляем вершины верхней стороны, сдвинутые вверх на ExtrudeHeight
	for (auto Vertex : BaseVertices)
	{
		auto local_vertex = Mesh->GetComponentTransform().InverseTransformPosition(Vertex);
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight) + FVector(0, 0, ExtrusionHeight));
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

void AMainTerrain::create_mesh_3d(UProceduralMeshComponent* MeshComponent, TArray<TSharedPtr<Node>> BaseVertices,
								  float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_point());
	}
	create_mesh_3d(MeshComponent, vertices, StarterHeight, ExtrusionHeight);
}
void AMainTerrain::create_mesh_3d(UProceduralMeshComponent* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
								  float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->point);
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}

void AMainTerrain::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight)
{
	int32 NumVertices = BaseVertices.Num();
	if (NumVertices < 3)
	{
		return; // Нужно хотя бы 3 вершины для создания полигона
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	// Добавляем вершины нижней стороны
	for (auto Vertex : BaseVertices)
	{
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().InverseTransformPosition(Vertex);
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight));
		// Vertices.Add(Vertex + FVector(0, 0, StarterHeight));
	}

	// Используем триангуляцию для нижней стороны
	TArray<int32> BaseTriangles;
	AllGeometry::TriangulatePolygon(Vertices, BaseTriangles);

	for (int32 i = 0; i < BaseTriangles.Num(); i += 3)
	{
		Triangles.Add(BaseTriangles[i]);
		Triangles.Add(BaseTriangles[i + 2]);
		Triangles.Add(BaseTriangles[i + 1]);
		// FVector point1 = Vertices[BaseTriangles[i]];
		// point1.Z = 0.8;
		// FVector point2 = Vertices[BaseTriangles[i + 1]];
		// point2.Z = 0.8;
		// FVector point3 = Vertices[BaseTriangles[i + 2]];
		// point3.Z = 0.8;
		// UE_LOG(LogTemp, Warning, TEXT("      "));
		// UE_LOG(LogTemp, Warning, TEXT("---Точка X %f, Y %f"), point1.X, point1.Y);
		// UE_LOG(LogTemp, Warning, TEXT("---Точка X %f, Y %f"), point2.X, point2.Y);
		// UE_LOG(LogTemp, Warning, TEXT("---Точка X %f, Y %f"), point3.X, point3.Y);
		//
		// DrawDebugLine(GetWorld(), point1, point3, FColor::Blue, true, -1, 0, 1);
		// DrawDebugLine(GetWorld(), point2, point3, FColor::Blue, true, -1, 0, 1);
		// DrawDebugLine(GetWorld(), point1, point2, FColor::Blue, true, -1, 0, 1);
	}

	Mesh->Vertices = Vertices;
	Mesh->Triangles = Triangles;
	// Создаем пустые массивы для нормалей, UV-координат и тангенсов
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	VerticesRemembered = Vertices;

	// Создаем меш
	Mesh->ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents,
														true);
	return;
}
void AMainTerrain::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
								  float StarterHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_point());
	}
	create_mesh_2d(Mesh, vertices, StarterHeight);
}
void AMainTerrain::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
								  float StarterHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->point);
	}
	create_mesh_2d(Mesh, vertices, StarterHeight);
}

void AMainTerrain::draw_all_3d()
{
	FlushPersistentDebugLines(GetWorld());
	BaseComponent = NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
	BaseComponent->SetupAttachment(RootComponent);
	BaseComponent->RegisterComponent();

	// Включаем коллизию
	BaseComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BaseComponent->SetCollisionResponseToAllChannels(ECR_Block);

	// Задаем сложную коллизию (на основе самих треугольников)
	BaseComponent->bUseComplexAsSimpleCollision = true;

	// Создаем физическое тело для коллизии
	create_mesh_3d(BaseComponent, map_borders_array, 0, 1);


	//
	// for (auto r : river)
	// {
	// 	for (auto rconn : r->conn)
	// 	{
	// 		DrawDebugLine(GetWorld(), rconn->node->get_point(), r->get_point(), FColor::Blue, true, -1, 0, 1);
	// 	}
	// }
	//
	// for (int i = 0; i < roads.Num(); i++)
	// {
	// 	for (int j = 0; j < roads[i]->conn.Num(); j++)
	// 	{
	// 		FColor color = FColor::Green;
	// 		DrawDebugLine(GetWorld(), roads[i]->conn[j]->node->get_point(), roads[i]->get_point(), color, true, -1, 0,
	// 					  1);
	// 	}
	// }


	// MeshComponent2->SetMaterial(NULL, LuxuryMaterial);

	for (auto& r : figures_array)
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
			// MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			// MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutLuxury);

			MeshComponent2->SetMaterial(NULL, LuxuryMaterial);
			create_mesh_3d(MeshComponent2, figure_to_print, 1, 0.5);
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
			// MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			// MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutDock);

			MeshComponent2->SetMaterial(NULL, DocsMaterial);
			create_mesh_3d(MeshComponent2, figure_to_print, 1, 0.5);
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
			// MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			// MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutRoyal);

			MeshComponent2->SetMaterial(NULL, RoyalMaterial);
			create_mesh_3d(MeshComponent2, figure_to_print, 1, 0.5);
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
			// MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			// MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutSlums);
			MeshComponent2->SetMaterial(NULL, SlumsMaterial);
			create_mesh_3d(MeshComponent2, figure_to_print, 1, 0.5);
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
			// MeshComponent2->OnBeginCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOver);
			// MeshComponent2->OnEndCursorOver.AddDynamic(this, &AMainTerrain::OnMouseOutResidential);
			MeshComponent2->SetMaterial(NULL, ResidenceMaterial);
			create_mesh_3d(MeshComponent2, figure_to_print, 1, 0.5);
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
			create_mesh_3d(MeshComponent2, p.house_figure, 0, p.height);
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
		if (!river_figure.figure.IsEmpty())
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
			create_mesh_3d(MeshComponent2, river_figure.figure, 1, 0.8);
		}
	}
}
void AMainTerrain::draw_all_2d()
{
	FlushPersistentDebugLines(GetWorld());

	// for (auto b : map_borders_array)
	// {
	// 	for (auto bconn : b->conn)
	// 	{
	// 		DrawDebugLine(GetWorld(), bconn->node->get_point(), b->get_point(), FColor::White, true, -1, 0, 20);
	// 	}
	// }


	for (auto b : roads)
	{
		for (auto bconn : b->conn)
		{
			auto start_point = b->get_point();
			auto end_point = bconn->node->get_point();
			start_point.Z = 0.8f;
			end_point.Z = 0.8f;
			DrawDebugLine(GetWorld(), start_point, end_point, FColor::Black, true, -1, 0, 1);
		}
	}

	AProceduralBlockMeshActor* Base =
		GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());

	// AProceduralBlockMeshActor Base;

	// Создаем физическое тело для коллизии
	create_mesh_2d(Base, map_borders_array, 0);

	for (auto& dp : debug_points_array)
	{
		FVector down_point = dp;
		FVector up_point = dp;
		down_point.Z = -20;
		up_point.Z = 20;
		DrawDebugLine(GetWorld(), down_point, up_point, FColor::Red, true, -1, 0, 1);
	}
	//
	// for (auto r : river)
	// {
	// 	for (auto rconn : r->conn)
	// 	{
	// 		DrawDebugLine(GetWorld(), rconn->node->get_point(), r->get_point(), FColor::Blue, true, -1, 0, 1);
	// 	}
	// }
	//
	// for (int i = 0; i < roads.Num(); i++)
	// {
	// 	for (int j = 0; j < roads[i]->conn.Num(); j++)
	// 	{
	// 		FColor color = FColor::Green;
	// 		DrawDebugLine(GetWorld(), roads[i]->conn[j]->node->get_point(), roads[i]->get_point(), color, true, -1, 0,
	// 					  1);
	// 	}
	// }


	// MeshComponent2->SetMaterial(NULL, LuxuryMaterial);

	for (auto& r : figures_array)
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
		// AProceduralBlockMeshActor* MeshComponent2 =
		// 	GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
		// MeshComponent2->ProceduralMesh->SetMaterial(NULL, LuxuryMaterial);
		// MeshComponent2->Material = LuxuryMaterial;
		// MeshComponent2->DefaultMaterial = BaseMaterial;
		// create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		if (r.get_type() == block_type::luxury)
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, LuxuryMaterial);
			MeshComponent2->Material = LuxuryMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		}
		else if (r.get_type() == block_type::dock)
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, DocsMaterial);
			MeshComponent2->Material = DocsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.02);
		}
		else if (r.get_type() == block_type::royal)
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, RoyalMaterial);
			MeshComponent2->Material = RoyalMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.03);
		}
		else if (r.get_type() == block_type::slums)
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, SlumsMaterial);
			MeshComponent2->Material = SlumsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.04);
		}
		else if (r.get_type() == block_type::residential)
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, ResidenceMaterial);
			MeshComponent2->Material = ResidenceMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.05);
		}

		for (auto& p : r.houses)
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, WaterMaterial);
			MeshComponent2->Material = WaterMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, p.house_figure, 0.06);
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
		if (!river_figure.figure.IsEmpty())
		{
			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, WaterMaterial);
			MeshComponent2->Material = WaterMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, river_figure.figure, 0.07);
		}
	}
}
void AMainTerrain::get_cursor_hit_location()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	// Получаем координаты мыши на экране
	float MouseX, MouseY;
	if (PlayerController->GetMousePosition(MouseX, MouseY))
	{
		FVector WorldLocation, WorldDirection;

		// Преобразуем координаты мыши в направление в мире
		if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
		{
			FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
			FVector CameraForwardVector = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
			FVector Start = CameraLocation; // Начальная точка линии (например, от камеры)
			FVector End = Start + (CameraForwardVector * 10000.0f); // Конечная точка линии

			// FVector HitLocation = HitResult.Location;
			// FVector HitWatch = HitLocation;
			// DrawDebugString(GetWorld(), HitWatch, HitLocation.ToString(), nullptr, FColor::Red, 50.0f,
			// 				true);
			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this); // Игнорировать самого себя

			bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End,
															 ECC_Visibility // Канал трассировки
			);

			if (bHit && HitResult.Component == BaseComponent)
			{
				// Логируем координаты попадания
				FVector HitLocation = HitResult.Location;
				FVector HitWatch = HitLocation;
				HitWatch.Z += 200;
				// UE_LOG(LogTemp, Warning, TEXT("Hit Location: %s"), *HitLocation.ToString());
				if (GEngine)
				{
					FString Message = FString::Printf(TEXT("Hit Location: %s"), *HitLocation.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Message);
					// DrawDebugString(GetWorld(), HitWatch, HitLocation.ToString(), nullptr, FColor::Red, 1.0f, true);
				}
			}
		}
	}
}
