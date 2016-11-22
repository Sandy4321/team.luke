#include "MultiIntegralDiscretizer.h"
#include <algorithm>
#include <cmath>
#include "Dataframe.h"
#include "InstanceCategorizer.h"
#include "Gain.h"



MultiIntegralDiscretizer::MultiIntegralDiscretizer(
	size_t attributeIdx, size_t answerIdx)
	: _attributeIdx(attributeIdx)
	, _answerIdx(answerIdx)
{
}


MultiIntegralDiscretizer::~MultiIntegralDiscretizer()
{
}

std::string MultiIntegralDiscretizer::Discretize(Instance* instance)
{
	for (size_t i = 0; i < _cutPoints.size(); ++i)
	{
		if (instance->GetAttribute(_attributeIdx).AsDouble() < _cutPoints[i])
			return std::string("Category ") + char('0' + i);
	}
	return std::string("Category ") + char('0' + _cutPoints.size());

}

void MultiIntegralDiscretizer::Build(const std::vector<Instance*>& instances)
{
	InstanceCategorizer parentCategorizer(_answerIdx);
	for (Instance* instance : instances)
		parentCategorizer.Add(instance);

	Split(parentCategorizer);
}

bool MultiIntegralDiscretizer::ShouldAccept(
	double gain, 
	InstanceCategorizer& total, 
	InstanceCategorizer& left, 
	InstanceCategorizer& right)
{
	int n = int(total.GetInstances().size());

	int k = int(total.GetClassCount());
	int k1 = int(left.GetClassCount());
	int k2 = int(right.GetClassCount());

	std::vector<int> s = std::move(total.GetEntropyVector());
	std::vector<int> s1 = std::move(left.GetEntropyVector());
	std::vector<int> s2 = std::move(right.GetEntropyVector());

	double entropy_total = Info(nullptr, "", "", s);
	double entropy_s1 = Info(nullptr, "", "", s1);
	double entropy_s2 = Info(nullptr, "", "", s2);

	return gain >
		(
			(log2l(n - 1) +
			log2l(powl(3, k) - 2.0) -
			(k*entropy_total + k1 * entropy_s1 + k2 * entropy_s2))/n
		);
}

// extract only necessary attributes (desired attribute & answer attribute)
struct ReducedInstanceRef
{
	size_t idx;
	double value;
	std::string answer;
};

void MultiIntegralDiscretizer::Split(
	InstanceCategorizer& totalCategorizer)
{
	std::vector<Instance*> instances =
		std::move(totalCategorizer.GetInstances());

	std::vector<ReducedInstanceRef> extractedAttributes;
	extractedAttributes.reserve(instances.size());
	for (size_t i = 0; i < instances.size(); ++i)
	{
		extractedAttributes.push_back({
			i,
			instances[i]->GetAttribute(_attributeIdx).AsDouble(),
			instances[i]->GetAttribute(_answerIdx).AsString() });
	}

	// sort instances by attribute
	std::sort(extractedAttributes.begin(), extractedAttributes.end(),
		[](ReducedInstanceRef& a, ReducedInstanceRef& b) {
		return a.value < b.value; });

	std::vector<int> parentEntropyVec =
		std::move(totalCategorizer.GetEntropyVector());

	// try all the possible cut points and choose the best cut point.
	double prevValue = extractedAttributes[0].value;
	double bestGain = -1.0;
	double bestGainCutpoint = std::numeric_limits<double>::max();
	InstanceCategorizer bestGainLeftChildCategorizer(_answerIdx);
	InstanceCategorizer bestGainRightChildCategorizer(_answerIdx);
	
	// prepare left and right sets
	std::vector<Instance*> left = { instances[extractedAttributes[0].idx] };
	left.reserve(extractedAttributes.size() - 1);
	std::vector<Instance*> right; right.reserve(extractedAttributes.size() - 1);
	for (auto it = extractedAttributes.rbegin();
		it != extractedAttributes.rend() - 1; ++it)
	{
		right.push_back(instances[it->idx]);
	}

	for (size_t i = 1; i < extractedAttributes.size(); ++i)
	{
		if (prevValue != extractedAttributes[i].value)
		{
			double cutpoint = (prevValue + extractedAttributes[i].value) * 0.5;
			
			InstanceCategorizer leftChildCategorizer(_answerIdx);
			for (auto instance : left)
				leftChildCategorizer.Add(instance);

			InstanceCategorizer rightChildCategorizer(_answerIdx);
			for (auto instance : right)
				rightChildCategorizer.Add(instance);

			std::vector<std::vector<int>> childEntropyVecs = {
				leftChildCategorizer.GetEntropyVector(),
				rightChildCategorizer.GetEntropyVector()
			};

			double gain = Gain(
				nullptr, "", parentEntropyVec,
				{ "" }, childEntropyVecs,
				false, "");

			if (gain > bestGain)
			{
				bestGain = gain;
				bestGainCutpoint = cutpoint;
				bestGainLeftChildCategorizer = std::move(leftChildCategorizer);
				bestGainRightChildCategorizer = std::move(rightChildCategorizer);
			}

			prevValue = extractedAttributes[i].value;
		}

		left.push_back(instances[extractedAttributes[i].idx]);
		right.pop_back();
	}

	// decide whether to cut or not to cut
	if (ShouldAccept(
		bestGain, totalCategorizer,
		bestGainLeftChildCategorizer, bestGainRightChildCategorizer))
	{
		if(bestGainLeftChildCategorizer.GetClassCount() > 1)
			Split(bestGainLeftChildCategorizer);

		_cutPoints.push_back(bestGainCutpoint);

		if (bestGainRightChildCategorizer.GetClassCount() > 1)
			Split(bestGainRightChildCategorizer);
	}
}