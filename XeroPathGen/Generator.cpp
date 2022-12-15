#include "Generator.h"
#include "RobotPath.h"
#include "PathGroup.h"
#include "CheesyGenerator.h"
#include "TrajectoryNames.h"
#include <QtCore/QThread>

Generator::Generator(double timestep, std::shared_ptr<RobotParams> robot, std::shared_ptr<TrajectoryGroup> group)
{
	timestep_ = timestep;
	group_ = group;
	robot_ = robot ;
}

void Generator::generateTrajectory()
{
	std::shared_ptr<PathTrajectory> traj;

	auto path = group_->path();
	if (group_->type() == GeneratorType::CheesyPoofs) {
		double diststep = UnitConverter::convert(1.0, "in", path->units());			// 1 inch works well, convert to units being used
		double maxdx = UnitConverter::convert(2.0, "in", path->units());			// 2 inches works well, convert to units being used
		double maxdy = UnitConverter::convert(0.5, "in", path->units());			// 0.5 inches works well, convert to units being used
		double maxtheta = 0.1;														// 0.1 radians works well, we don't convert angles

		CheesyGenerator gen(diststep, timestep_, maxdx, maxdy, maxtheta);
		auto traj = gen.generate(path->waypoints(), path->constraints(), path->params().startVelocity(),
			path->params().endVelocity(), path->params().maxVelocity(), path->params().maxAccel(), 0.0);

		group_->addTrajectory(traj);
	}
	else if (group_->type() == GeneratorType::ErrorCodeXeroSwerve) {
		group_->setErrorMessage("generator type 'ErrorCodeXeroSwerve' not supported (yet)");
	}

	if (!group_->hasError()) {
		if (robot_->getDriveType() == RobotParams::DriveType::TankDrive) {
			//
			// Add in trajectories for the left and right wheels
			//
		}
	}

	emit trajectoryComplete(group_);
}

void Generator::addTankDriveTrajectories()
{
	//
	// Get the width of the robot in the same units used by the paths
	//
	double width = UnitConverter::convert(robot_->getEffectiveWidth(), robot_->getLengthUnits(), group_->path()->units());

	auto traj = group_->getTrajectory(TrajectoryName::Main);
	if (traj == nullptr) {
		return;
	}

	QVector<Pose2dWithTrajectory> leftpts;
	QVector<Pose2dWithTrajectory> rightpts;

	double lcurv = 0, rcurv = 0;
	double lvel = 0, lacc = 0, lpos = 0, ljerk = 0;
	double rvel = 0, racc = 0, rpos = 0, rjerk = 0;
	double plx = 0, ply = 0, prx = 0, pry = 0;
	double plvel = 0, prvel = 0;
	double placc = 0, pracc = 0;

	for (size_t i = 0; i < traj->size(); i++)
	{
		const Pose2dWithTrajectory& pt = (*traj)[i];
		double time = pt.time();
		double px = pt.x();
		double py = pt.y();

		double lx = px - width * pt.rotation().getSin() / 2.0;
		double ly = py + width * pt.rotation().getCos() / 2.0;
		double rx = px + width * pt.rotation().getSin() / 2.0;
		double ry = py - width * pt.rotation().getCos() / 2.0;

		if (i == 0)
		{
			lvel = 0.0;
			lacc = 0.0;
			lpos = 0.0;
			ljerk = 0.0;

			rvel = 0.0;
			racc = 0.0;
			rpos = 0.0;
			rjerk = 0.0;
		}
		else
		{
			double dt = time - (*traj)[i - 1].time();
			double ldist = std::sqrt((lx - plx) * (lx - plx) + (ly - ply) * (ly - ply));
			double rdist = std::sqrt((rx - prx) * (rx - prx) + (ry - pry) * (ry - pry));

			lvel = ldist / dt;
			rvel = rdist / dt;

			lacc = (lvel - plvel) / dt;
			racc = (rvel - prvel) / dt;

			ljerk = (lacc - placc) / dt;
			rjerk = (racc - pracc) / dt;

			lpos += ldist;
			rpos += rdist;
		}

		Translation2d lpt(lx, ly);
		Pose2d l2d(lpt, pt.rotation());
		Pose2dWithTrajectory ltraj(l2d, time, lpos, lvel, lacc, ljerk, lcurv, 0.0);
		leftpts.push_back(ltraj);

		Translation2d rpt(rx, ry);
		Pose2d r2d(rpt, pt.rotation());
		Pose2dWithTrajectory rtraj(r2d, time, rpos, rvel, racc, rjerk, rcurv, 0.0);
		rightpts.push_back(rtraj);

		plx = lx;
		ply = ly;
		prx = rx;
		pry = ry;
		plvel = lvel;
		prvel = rvel;
		placc = lacc;
		pracc = racc;
	}

	assert(leftpts.size() == rightpts.size());
	for (int i = 0; i < leftpts.size(); i++) {
		if (i == 0 || i == leftpts.size() - 1)
		{
			lcurv = 0.0;
			rcurv = 0.0;
		}
		else
		{
			leftpts[i].setCurvature(Pose2dWithRotation::curvature(leftpts[i - 1].pose(), leftpts[i].pose(), leftpts[i + 1].pose()));
			rightpts[i].setCurvature(Pose2dWithRotation::curvature(rightpts[i - 1].pose(), rightpts[i].pose(), rightpts[i + 1].pose()));
		}
	}

	std::shared_ptr<PathTrajectory> left = std::make_shared<PathTrajectory>(TrajectoryName::Left, leftpts);
	std::shared_ptr<PathTrajectory> right = std::make_shared<PathTrajectory>(TrajectoryName::Right, rightpts);

	group_->addTrajectory(left);
	group_->addTrajectory(right);
}