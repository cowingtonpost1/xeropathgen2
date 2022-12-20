#pragma once

#include "Pose2dWithRotation.h"
#include "PathConstraint.h"
#include "PathParameters.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QJsonObject>
#include <memory>

class PathGroup;

class RobotPath : public QObject
{
	friend class CentripetalConstraint;
	friend class DistanceVelocityConstraint;
	friend class PathsDataModel;

	Q_OBJECT

public:
	static constexpr const char* VersionTag = "_version";
	static constexpr const char* UnitsTag = "units";
	static constexpr const char* GeneratorTag = "generator";
	static constexpr const char* TimeTag = "time";
	static constexpr const char* OutputTag = "outdir";
	static constexpr const char* GroupsTag = "groups";
	static constexpr const char* GroupTag = "group";
	static constexpr const char* PositionTag = "position";
	static constexpr const char* CurvatureTag = "curvature";
	static constexpr const char* AccelerationTag = "acceleration";
	static constexpr const char* NameTag = "name";
	static constexpr const char* PathsTag = "paths";
	static constexpr const char* StartVelocityTag = "startvelocity";
	static constexpr const char* EndVelocityTag = "endvelocity";
	static constexpr const char* StartAngleTag = "startangle";
	static constexpr const char* StartAngleDelayTag = "startangledelay";
	static constexpr const char* EndAngleTag = "endangle";
	static constexpr const char* EndAngleDelayTag = "endangledelay";
	static constexpr const char* MaxVelocityTag = "maxvelocity";
	static constexpr const char* MaxAccelerationTag = "maxacceleration";
	static constexpr const char* MaxCentripetalTag = "maxcentripetal";
	static constexpr const char* ConstraintsTag = "constraints";
	static constexpr const char* TypeTag = "type";
	static constexpr const char* DistanceVelocityTag = "distance_velocity";
	static constexpr const char* BeforeTag = "before";
	static constexpr const char* AfterTag = "after";
	static constexpr const char* VelocityTag = "velocity";
	static constexpr const char* PointsTag = "points";
	static constexpr const char* XTag = "x";
	static constexpr const char* YTag = "y";
	static constexpr const char* SwerveRotationTag = "swrot";
	static constexpr const char* HeadingTag = "heading";
	static constexpr const char* RotationTag = "rotation";

public:
	RobotPath(const PathGroup *gr, const QString &units, const QString &name, const PathParameters &params);
	RobotPath(const PathGroup* gr, const RobotPath& path);

	const QString& name() const {
		return name_;
	}

	void setName(const QString& name) {
		emitBeforePathChangedSignal();
		name_ = name;
		emitAfterPathChangedSignal();
	}

	const QString& units() const {
		return units_;
	}

	const PathParameters& params() const {
		return params_;
	}

	void setParams(const PathParameters& p) {
		emitBeforePathChangedSignal();
		params_ = p;
		emitBeforePathChangedSignal();
	}

	const PathGroup* pathGroup() const {
		return group_;
	}

	const QVector<Pose2dWithRotation> waypoints() const {
		return waypoints_;
	}

	void addWayPoint(const Pose2dWithRotation& waypoint) {
		emitBeforePathChangedSignal();
		waypoints_.push_back(waypoint);
		emitBeforePathChangedSignal();
	}

	bool isEmpty() const {
		return waypoints_.size() == 0;
	}

	size_t size() const {
		return waypoints_.size();
	}

	const Pose2dWithRotation& getPoint(size_t index) const {
		return waypoints_[index];
	}

	void replacePoint(size_t index, const Pose2dWithRotation& pt) {
		emitBeforePathChangedSignal();
		waypoints_[index] = pt;
		emitBeforePathChangedSignal();

	}

	void removePoint(size_t index) {
		emitBeforePathChangedSignal();
		waypoints_.remove(index, 1);
		emitBeforePathChangedSignal();

	}

	void insertPoint(size_t index, const Pose2dWithRotation& pt) {
		emitBeforePathChangedSignal();
		waypoints_.insert(index + 1, pt);
		emitBeforePathChangedSignal();
	}

	void addConstraint(std::shared_ptr<PathConstraint> c) {
		emitBeforePathChangedSignal();
		constraints_.push_back(c);
		emitBeforePathChangedSignal();
	}

	void deleteConstraint(std::shared_ptr<PathConstraint> c) {
		auto it = std::find(constraints_.begin(), constraints_.end(), c);
		if (it != constraints_.end()) {
			emitBeforePathChangedSignal();
			constraints_.erase(it);
			emitBeforePathChangedSignal();
		}
	}

	void beforeConstraintChanged() {
		emitBeforePathChangedSignal();
	}

	void afterConstraintChanged() {
		emitAfterPathChangedSignal();
	}

	const QVector<std::shared_ptr<PathConstraint>>& constraints() const {
		return constraints_;
	}

	void convert(const QString& from, const QString& to);

	static std::shared_ptr<RobotPath> fromJSONObject(const PathGroup *group, const QString &units, const QJsonObject& obj, QString &msg);
	QJsonObject toJSONObject();

signals:
	void afterPathChanged(const QString& groupName, const QString& pathName);
	void beforePathChanged(const QString& groupName, const QString& pathName);

private:
	void emitBeforePathChangedSignal();
	void emitAfterPathChangedSignal();

	static bool readPoints(std::shared_ptr<RobotPath> path, const QJsonArray& obj, QString &msg);
	static bool readConstraints(std::shared_ptr<RobotPath> path, const QJsonArray& obj, QString &msg);
	static double getDoubleParam(const QJsonObject& obj, const QString& tag, QString& msg);
	static QString getStringParam(const QJsonObject& obj, const QString& tag, QString& msg);
	static QJsonObject getObjectParam(const QJsonObject& obj, const QString& tag, QString& msg);
	static QJsonArray getArrayParam(const QJsonObject& obj, const QString& tag, QString& msg);

private:
	const PathGroup* group_;										// The group this path belongs to
	QString name_;													// The name of the path
	QVector<Pose2dWithRotation> waypoints_;							// The waypoints along the path including robot motion heading and swerve rotation
	QVector<std::shared_ptr<PathConstraint>> constraints_;			// The set of constrains to apply to the path
	PathParameters params_;											// The path velocity and acceleration parameters
	QString units_;													// The units for this path
};
