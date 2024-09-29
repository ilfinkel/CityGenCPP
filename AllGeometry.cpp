// Fill out your copyright notice in the Description page of Project Settings.


#include "AllGeometry.h"

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
	const TSharedPtr<Node>& line1_begin, const TSharedPtr<Node>& line1_end, const TArray<TSharedPtr<Node>>& lines,
	bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point = is_intersect(line1_begin->get_point(), line1_end->get_point(),
														line->get_point(), conn->node->get_point(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line1_begin->get_point(), int_point.GetValue());
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
	FVector line1_begin, FVector line1_end, const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point =
				is_intersect(line1_begin, line1_end, line->get_point(), conn->node->get_point(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line1_begin, int_point.GetValue());
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


TOptional<TSharedPtr<Node>> AllGeometry::is_intersect_array_clear(const TSharedPtr<Node>& line1_begin,
																  const TSharedPtr<Node>& line1_end,
																  const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<TSharedPtr<Node>>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->get_point()) <
			FVector::Dist(inter_segment->Key, inter_segment->Value.Value->get_point())
		? inter_segment->Value.Key
		: inter_segment->Value.Value;
}

TOptional<FVector> AllGeometry::is_intersect_array_clear(const FVector& line1_begin, const FVector& line1_end,
														 const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
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

int AllGeometry::calculate_angle(const FVector& A, const FVector& B, const FVector& C, bool is_clockwork)
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
		FVector CrossProduct = FVector::CrossProduct(BA, BC);
		if (CrossProduct.Z > 0)
		{
			AngleRadians = 2 * PI - AngleRadians;
		}
	}
	double AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}
