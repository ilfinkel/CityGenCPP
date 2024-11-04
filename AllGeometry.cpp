﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "AllGeometry.h"

#include "NavMesh/RecastNavMesh.h"

Block::Block(TArray<TSharedPtr<Point>> figure_)
{
	// figure = figure_;
	bool is_found;
	if (figure_.Num() > 3)
	{
		do
		{
			int beg_del = 0;
			int end_del = 0;
			is_found = false;
			for (int i = 0; i < figure_.Num(); i++)
			{
				for (int j = i + 2; j < figure_.Num(); j++)
				{
					if (figure_[i] == figure_[j] && figure_[i + 1] == figure_[j - 1])
					{
						beg_del = i;
						end_del = j;
						is_found = true;
						break;
					}
				}
				if (is_found)
				{
					break;
				}
			}
			figure_.RemoveAt(beg_del, end_del - beg_del);
		}
		while (is_found);
	}
	figure = figure_;

	area = AllGeometry::get_poygon_area(figure);
	type = block_type::unknown;
	if (area < 50000)
	{
		set_type(block_type::empty);
	}
	// TSet<block_type> first_node(TSet<block_type>(figure[0]->blocks_nearby));
	// for (auto a : figure[0]->blocks_nearby)
	// {
	// 	first_node.Add(a);
	// }
	// for (int i = 1; i < figure.Num(); i++)
	// {
	// 	first_node.Intersect(TSet<block_type>(figure[i]->blocks_nearby));
	// 	if (first_node.IsEmpty())
	// 	{
	// 		break;
	// 	}
	// }
	// if (!first_node.IsEmpty())
	// {
	// 	set_type(*first_node.CreateConstIterator());
	// }


	// AllGeometry::change_size(figure, 0.7f);
}

void Block::set_type(block_type type_)
{
	type = type_;
	for (int i = 0; i < figure.Num() - 2; i++)
	{
		figure[i]->blocks_nearby.Add(type_);
	}
}
bool Block::is_point_in_figure(TSharedPtr<Point> point_)
{
	FVector point = point_->point;
	FVector point2 = point_->point;
	point2.Y = y_size;
	int times_to_hit = 0;
	for (int i = 1; i < figure.Num(); i++)
	{
		if (AllGeometry::is_intersect(point, point2, figure[i - 1]->point, figure[i]->point, true).IsSet())
		{
			times_to_hit++;
		}
	}
	if (times_to_hit % 2 == 1)
	{
		return true;
	}
	return false;
}
void Block::get_self_figure()
{
	for (auto fig : figure)
	{
		Point p = *fig;
		self_figure.Add(p);
	}
}

void Block::shrink_size(TArray<Point>& Vertices, float size_delta)
{
	int32 NumVertices = Vertices.Num();

	if (NumVertices < 3 || size_delta <= 0.0f)
	{
		return;
	}
	TArray<FVector> new_vertices;
	for (int i = 1; i <= NumVertices; ++i)
	{
		double angle = AllGeometry::calculate_angle(Vertices[i - 1].point, Vertices[i % NumVertices].point,
													Vertices[(i + 1) % NumVertices].point, true);
		FVector new_point =
			AllGeometry::create_segment_at_angle(Vertices[i - 1].point, Vertices[i % NumVertices].point,
												 Vertices[i % NumVertices].point, angle / 2, size_delta);

		new_vertices.Add(new_point);
	}
	for (int i = 0; i < NumVertices; ++i)
	{
		Vertices[i].point = new_vertices[i];
	}
}
TOptional<FVector> Block::is_line_intersect(FVector point1, FVector point2)
{
	int NumVertices = self_figure.Num();
	for (int i = 1; i <= NumVertices; i++)
	{
		TOptional<FVector> intersect = AllGeometry::is_intersect(point1, point2, self_figure[i - 1].point,
																 self_figure[i % NumVertices].point, false);
		if (intersect.IsSet())
		{
			return intersect;
		}
	}
	return FVector();
}

TOptional<TSharedPtr<Conn>> Node::get_next_point(TSharedPtr<Point> point)
{
	for (auto c : conn)
	{
		if (c->node->get_point() == point->point)
		{
			return c;
		}
	}
	return TOptional<TSharedPtr<Conn>>();
}

TOptional<TSharedPtr<Conn>> Node::get_prev_point(TSharedPtr<Point> point)
{
	auto prev_point = get_next_point(point);
	if (prev_point.IsSet())
	{
		return prev_point.GetValue()->node->get_next_point(node);
	}
	return TOptional<TSharedPtr<Conn>>();
}

void Node::add_connection(const TSharedPtr<Node>& node_) { conn.Add(MakeShared<Conn>(Conn(node_))); }
void Node::delete_me()
{
	for (auto c : conn)
	{
		for (int i = 0; i < c->node->conn.Num(); i++)
		{
			if (node->point == c->node->conn[i]->node->get_point())
			{
				c->node->conn.RemoveAt(i);
				break;
			}
		}
	}
}


TOptional<FVector> AllGeometry::is_intersect(const FVector& line1_begin, const FVector& line1_end,
											 const FVector& line2_begin, const FVector& line2_end, bool is_opened)
{
	double dx1 = line1_end.X - line1_begin.X;
	double dy1 = line1_end.Y - line1_begin.Y;
	double dx2 = line2_end.X - line2_begin.X;
	double dy2 = line2_end.Y - line2_begin.Y;

	double det = dx1 * dy2 - dx2 * dy1;

	if (std::abs(det) < 1e-6)
	{
		return TOptional<FVector>();
	}

	double t1 = ((line2_begin.X - line1_begin.X) * dy2 - (line2_begin.Y - line1_begin.Y) * dx2) / det;
	double t2 = ((line2_begin.X - line1_begin.X) * dy1 - (line2_begin.Y - line1_begin.Y) * dx1) / det;

	if (t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0)
	{
		FVector intersectionPoint(line1_begin.X + t1 * dx1, line1_begin.Y + t1 * dy1, 0);
		if (is_opened)
		{
			return intersectionPoint;
		}
		if (!is_opened &&
			(FVector::Distance(intersectionPoint, line2_begin) > TNumericLimits<double>::Min() &&
			 FVector::Distance(intersectionPoint, line2_end) > TNumericLimits<double>::Min() &&
			 FVector::Distance(intersectionPoint, line1_begin) > TNumericLimits<double>::Min() &&
			 FVector::Distance(intersectionPoint, line1_end) > TNumericLimits<double>::Min()))
		{
			return intersectionPoint;
		}
	}

	return TOptional<FVector>();
}


TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> AllGeometry::is_intersect_array(
	const TSharedPtr<Node>& line_begin, const TSharedPtr<Node>& line_end, const TArray<TSharedPtr<Node>>& lines,
	bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point = is_intersect(line_begin->get_point(), line_end->get_point(),
														line->get_point(), conn->node->get_point(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line_begin->get_point(), int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{line, conn->node};

					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{intersect_point_final, point_line};
	return final_tuple;
}


TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> AllGeometry::is_intersect_array(
	FVector line_begin, FVector line_end, const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point =
				is_intersect(line_begin, line_end, line->get_point(), conn->node->get_point(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line_begin, int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{line, conn->node};

					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{intersect_point_final, point_line};
	return final_tuple;
}


TOptional<TSharedPtr<Node>> AllGeometry::is_intersect_array_clear(const TSharedPtr<Node>& line_begin,
																  const TSharedPtr<Node>& line_end,
																  const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line_begin, line_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<TSharedPtr<Node>>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->get_point()) <
			FVector::Dist(inter_segment->Key, inter_segment->Value.Value->get_point())
		? inter_segment->Value.Key
		: inter_segment->Value.Value;
}
int AllGeometry::is_intersect_array_count(const TSharedPtr<Node>& line_begin, const TSharedPtr<Node>& line_end,
										  const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	int count = 0;
	for (auto l : lines)
	{
		for (auto& conn : l->conn)
		{
			if (is_intersect(line_begin->get_point(), line_end->get_point(), l->get_point(), conn->node->get_point(),
							 is_opened))
			{
				count++;
			}
		}
	}
	return count / 2;
}

TOptional<FVector> AllGeometry::is_intersect_array_clear(const FVector& line_begin, const FVector& line_end,
														 const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line_begin, line_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<FVector>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->get_point()) <
			FVector::Dist(inter_segment->Key, inter_segment->Value.Value->get_point())
		? inter_segment->Value.Key->get_point()
		: inter_segment->Value.Value->get_point();
}

FVector AllGeometry::create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
											 const FVector& line_beginPoint, double angle_in_degrees, double length)
{
	FVector line_direction = (line_end - line_begin).GetSafeNormal();
	FVector rotated_direction = line_direction.RotateAngleAxis(angle_in_degrees, FVector(0.f, 0.f, 1.f));
	FVector line_endPoint = line_beginPoint + rotated_direction * length;

	return line_endPoint;
}

double AllGeometry::calculate_angle(const FVector& A, const FVector& B, const FVector& C, bool is_clockwork)
{
	FVector BA = A - B;
	FVector BC = C - B;
	BA.Normalize();
	BC.Normalize();
	double CosTheta = FVector::DotProduct(BA, BC);
	CosTheta = FMath::Clamp(CosTheta, -1.0f, 1.0f);
	double AngleRadians = FMath::Acos(CosTheta);
	if (is_clockwork == true)
	{
		FVector CrossProduct = FVector::CrossProduct(BC, BA);
		if (CrossProduct.Z > 0)
		{
			AngleRadians = 2 * PI - AngleRadians;
		}
	}
	double AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

float AllGeometry::get_poygon_area(const TArray<TSharedPtr<Point>>& Vertices)
{
	int32 NumVertices = Vertices.Num();
	if (NumVertices < 3)
	{
		return 0.0f;
	}

	float Area = 0.0f;

	for (int32 i = 1; i < NumVertices; ++i)
	{
		const FVector& CurrentVertex = Vertices[i - 1]->point;
		const FVector& NextVertex = Vertices[i]->point;

		Area += FMath::Abs(CurrentVertex.X * NextVertex.Y - CurrentVertex.Y * NextVertex.X);
	}

	Area = Area / 2;

	return Area;
}
float AllGeometry::get_poygon_area(const TArray<Point>& Vertices)
{
	int32 NumVertices = Vertices.Num();
	if (NumVertices < 3)
	{
		return 0.0f;
	}

	float Area = 0.0f;

	for (int32 i = 1; i <= NumVertices; ++i)
	{
		const FVector& CurrentVertex = Vertices[i - 1].point;
		const FVector& NextVertex = Vertices[i % NumVertices].point;

		Area += FMath::Abs(CurrentVertex.X * NextVertex.Y - CurrentVertex.Y * NextVertex.X);
	}

	Area /= 2;

	return Area;
}

bool AllGeometry::IsConvex(const FVector& Prev, const FVector& Curr, const FVector& Next)
{
	// Проверка на выпуклость вершины
	FVector Edge1 = Curr - Prev;
	FVector Edge2 = Next - Curr;
	return FVector::CrossProduct(Edge1, Edge2).Z <= 0;
}
bool AllGeometry::IsPointInTriangle(const FVector& P, const FVector& A, const FVector& B, const FVector& C)
{
	FVector v0 = C - A;
	FVector v1 = B - A;
	FVector v2 = P - A;

	float dot00 = FVector::DotProduct(v0, v0);
	float dot01 = FVector::DotProduct(v0, v1);
	float dot02 = FVector::DotProduct(v0, v2);
	float dot11 = FVector::DotProduct(v1, v1);
	float dot12 = FVector::DotProduct(v1, v2);

	float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (u >= 0) && (v >= 0) && (u + v < 1);
}

bool AllGeometry::IsEar(const TArray<FVector>& Vertices, int32 PrevIndex, int32 CurrIndex, int32 NextIndex,
						const TArray<int32>& RemainingVertices)
{
	FVector A = Vertices[PrevIndex];
	FVector B = Vertices[CurrIndex];
	FVector C = Vertices[NextIndex];

	for (int32 Index : RemainingVertices)
	{
		if (Index != PrevIndex && Index != CurrIndex && Index != NextIndex)
		{
			if (IsPointInTriangle(Vertices[Index], A, B, C))
			{
				return false;
			}
		}
	}

	return true;
}

bool AllGeometry::IsPointInsidePolygon(const FVector& Point, const TArray<FVector>& Polygon)
{
	int32 Intersections = 0;
	FVector FarPoint = Point + FVector(0, 10000, 0); // Далеко расположенная точка на линии

	for (int32 i = 0; i < Polygon.Num(); i++)
	{
		FVector A = Polygon[i];
		FVector B = Polygon[(i + 1) % Polygon.Num()];

		if (AllGeometry::is_intersect(Point, FarPoint, A, B, true).IsSet())
		{
			Intersections++;
		}
	}

	return (Intersections % 2 == 1); // Проверка по четности
}
void AllGeometry::TriangulatePolygon(const TArray<FVector>& Vertices, TArray<int32>& Triangles)
{
	TArray<int32> RemainingVertices;
	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		RemainingVertices.Add(i);
	}

	while (RemainingVertices.Num() > 2)
	{
		bool EarFound = false;

		for (int32 i = 0; i < RemainingVertices.Num(); i++)
		{
			int32 PrevIndex = RemainingVertices[(i + RemainingVertices.Num() - 1) % RemainingVertices.Num()];
			int32 CurrIndex = RemainingVertices[i];
			int32 NextIndex = RemainingVertices[(i + 1) % RemainingVertices.Num()];

			// Центр треугольника для проверки
			FVector TriangleCenter = (Vertices[PrevIndex] + Vertices[CurrIndex] + Vertices[NextIndex]) / 3;

			// Проверка на то, что центр внутри фигуры
			if (IsPointInsidePolygon(TriangleCenter, Vertices) &&
				IsEar(Vertices, PrevIndex, CurrIndex, NextIndex, RemainingVertices))
			{
				// Добавление треугольника
				Triangles.Add(PrevIndex);
				Triangles.Add(CurrIndex);
				Triangles.Add(NextIndex);

				// Удаляем текущее ухо
				RemainingVertices.RemoveAt(i);
				EarFound = true;
				break;
			}
		}

		if (!EarFound)
		{
			break;
		}
	}
}
