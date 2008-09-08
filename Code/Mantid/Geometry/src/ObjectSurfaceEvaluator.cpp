#include "MantidGeometry/V3D.h"
#include "MantidGeometry/SurfaceEvaluator.h"
#include "MantidGeometry/ObjectSurfaceEvaluator.h"
#include "MantidGeometry/Rules.h"
#include <cassert>

namespace Mantid
{
	namespace Geometry
	{

		/**
		 * Evaluate the Rule tree, crude way of comparing the rule type and calling appropriate
		 * rule method.
		 * @param rTree input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(Rule* rTree,V3D point)
		{
			if(rTree->className()=="Intersection"){
				return evaluate((Intersection*)rTree,point);
			}else if(rTree->className()=="Union"){
				return evaluate((Union*)rTree,point);
			}else if(rTree->className()=="SurfPoint"){
				return evaluate((SurfPoint*)rTree,point);
			}else if(rTree->className()=="CompGrp"){
				return evaluate((CompGrp*)rTree,point);
			}else if(rTree->className()=="CompObj"){
				return evaluate((CompObj*)rTree,point);
			}else if(rTree->className()=="BoolValue"){
				return evaluate((BoolValue*)rTree,point);
			}
			return 0.0;
		}

		/**
		 * Evaluate the intersection rule, using R-Functions intersection is min of two nodes
		 * @param rule input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(Intersection* rule,V3D point)
		{
			double left=evaluate(rule->leaf(0),point);
			double right=evaluate(rule->leaf(1),point);
			if(left<right)return left;
			else return right;
		}

		/**
		 * Evaluate the union rule, using R-Functions intersection is max of two nodes
		 * @param rule input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(Union* rule,V3D point)
		{
			double left=evaluate(rule->leaf(0),point);
			double right=evaluate(rule->leaf(1),point);
			if(left>right)return left;
			else return right;
		}

		/**
		 * Evaluate the SurfPoint rule, call quadratic value of the equation at input point
		 * @param rule input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(SurfPoint* rule,V3D point)
		{
			double val=((Quadratic*)rule->getKey())->eqnValue(point);
			val*=rule->getSign();
			return val;
		}

		/**
		 * Evaluate the CompObj rule, compliment the object, R-Functions its simply a negation
		 * @param rule input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(CompObj* rule,V3D point)
		{
			Object* obj=rule->getObj();
			ObjectSurfaceEvaluator ose(obj);
			double val=ose.evaluate(point);
			val*=-1;
			return val;
		}

		/**
		 * Evaluate the CompObj rule, compliment the rule group, R-Functions its simply a negation
		 * @param rule input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(CompGrp* rule,V3D point)
		{
			double val=evaluate(rule->leaf(0),point);
			val*=-1;
			return val;
		}

		/**
		 * Evaluate the CompObj rule, boolean value, R-Functions there is nothing equivalent
		 * @param rule input rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(BoolValue* rule,V3D point)
		{
			return 0;
		}

		/**
		 * Evaluate the rule
		 * @param point point at which rule needs to be evaluated
		 */
		double ObjectSurfaceEvaluator::evaluate(V3D point)
		{
			return evaluate((Rule*)surf->topRule(),point);
		}
	}
}
