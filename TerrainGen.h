#pragma once

#include <City/AllGeometry.h>
#include <City/MainTerrain.h>

class TerrainGen
{
public:
	FVector center;
	double av_distance;
	double av_river_length;
	double max_river_length;
	double min_new_road_length;
	double min_road_length;
	double av_road_length;
	double max_road_length;
	double river_road_distance;
	double x_size;
	double y_size;
	ECityPlan city_plan;

	TArray<District> figures_array;
	District river_figure;
	TerrainGen(FMapParams& map_params) :
		center(map_params.center), av_distance(map_params.av_distance), av_river_length(map_params.av_river_length),
		max_river_length(map_params.max_river_length), min_new_road_length(map_params.min_new_road_length),
		min_road_length(map_params.min_road_length), av_road_length(map_params.av_road_length),
		max_road_length(map_params.max_road_length), river_road_distance(map_params.river_road_distance),
		x_size(map_params.x_size), y_size(map_params.y_size), city_plan(map_params.city_plan) {};

	void create_terrain(TArray<TSharedPtr<Node>>& roads_, TArray<District>& figures_array_, District& river_figure_,
						TArray<TSharedPtr<Node>>& map_borders_array_, TArray<FVector>& debug_points_array_);
	void add_conn(TSharedPtr<Node> node1, TSharedPtr<Node> node2);
	TSharedPtr<Node> insert_conn(TSharedPtr<Node> node1_to_insert, TSharedPtr<Node> node2_to_insert,
								 FVector node3_point);
	void move_river(TSharedPtr<Node>& node1, TSharedPtr<Node>& node2);
	void move_road(TSharedPtr<Node>& node);
	void create_guiding_rivers();
	void create_guiding_river_segment(TSharedPtr<Node> start_point, TSharedPtr<Node> end_point,
									  TSharedPtr<Node> start_point_left, TSharedPtr<Node> start_point_right);
	void process_bridges();
	void create_guiding_roads();
	void create_usual_roads();
	TOptional<TSharedPtr<Node>> create_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
											   TSharedPtr<Node> end_point, bool to_exect_point, point_type type,
											   double max_length);
	bool create_guiding_road_segment(TSharedPtr<Node>& start_point, TSharedPtr<Node>& end_point);
	void shrink_roads();
	void point_shift(FVector& point);
	void get_closed_figures(TArray<TSharedPtr<Node>> lines, TArray<District>& fig_array, int figure_threshold);
	void get_river_figure();
	void process_blocks(TArray<District>& blocks);
	void process_houses(District& block);
	TArray<TSharedPtr<Node>> river;
	TArray<TSharedPtr<Node>> guiding_river;
	TArray<TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> bridges;
	TArray<TSharedPtr<Node>> road_centers;
	TArray<FVector> map_points_array;
	TArray<TSharedPtr<Node>> map_borders_array;
	TArray<TSharedPtr<Node>> guididng_roads_array;
	TArray<WeightedPoint> weighted_points;
	TArray<TSharedPtr<Node>> roads;
};
