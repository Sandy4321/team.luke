#include "RecommenderSystemTest.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "../ML/Dataframe/Dataframe.h"
#include "../ML/Association/KmeansClustering.h"
#include "../ML/Recommender/RecommenderSystem.h"

// prints table to out stream in csv format
void PrintTable(std::ostream& out,
  Dataframe& dataframe,
  const std::vector<std::vector<double>>& table)
{
  // prints header
  for (size_t i = 1; i < dataframe.GetAttributeCount(); ++i)
  {
    out << ",";
    out << dataframe.GetAttributeName(i);
  }
  out << std::endl;

  // prints each user scores with their names
  for (size_t i = 0; i < dataframe.GetInstanceCount(); ++i)
  {
    // prints name
    const Instance& instance = dataframe.GetInstance(i);
    out << instance.GetAttribute(0).AsString();

    // prints product scores
    for (double col : table[i])
      out << "," << col;

    out << std::endl;
  }
}

void RecommenderSystemTest_Movies(
  const std::string& criticsFilename,
  const std::string& usersFilename,
  std::ostream* debug_out,
  std::ostream* out)
{
  // build critics dataframe
  Dataframe critics;
  if (!critics.BuildFromCsv(criticsFilename, true))
  {
    if (debug_out)
      *debug_out << "moviedata.csv doesn't exist!" << std::endl;
    return;
  }

  // build users dataframe
  Dataframe users;
  if (!users.BuildFromCsv(usersFilename, true))
  {
    if (debug_out)
      *debug_out << "movieata_test.csv doesn't exist!" << std::endl;
    return;
  }

  // set debug output stream for detailed view
  RecommenderSystem recommender(critics);
  recommender.SetDebugOutput(debug_out);

  // run recommender system for the given users and prints the result
  std::vector<std::vector<double>>&& table = recommender.RecommendFor(users);
  if (out)
    PrintTable(*out, users, table);
}

void RecommenderSystemTest_Wines(const std::string & winefilename, std::ostream * debug_out, std::ostream & out)
{

  // build critics dataframe
  Dataframe wines;
  if (!wines.BuildFromCsv(winefilename, true))
  {
    if (debug_out)
      *debug_out << "file doesn't exist!" << std::endl;
    return;
  }

# define GROUP 2
# define PRECISION 0.8

  KMeansClustering mKMC(wines);
  ClusterData clustered = mKMC.Cluster(GROUP);

  typedef std::pair<unsigned, double>  AnswerForm;
  typedef std::vector<AnswerForm>      Answer;
  typedef std::vector<Answer>          Answers;
  Answers mRecommends;

# define QUALITY "quality"

  int ignore = wines.GetAttributeIndex(std::string(QUALITY));
  std::vector<DataPoint> mTable;
  mRecommends.resize(GROUP);
  {
    std::vector<double> cluster_len;

    // assign each instance to a cluster whose centroid is closest to it
    auto& instances = wines.GetInstances();
    mTable.resize(instances.size());
    for (unsigned index = 0; index < instances.size(); ++index)
    {
      DataPoint point = mKMC.ToDataPoint(instances[index]);
      double point_len = KMeansClustering::Length(point, { ignore });

      for (int i = 0, size = static_cast<int>(clustered.size());
        i < size; ++i)
      {
        const auto & cluster = clustered[i];
        if (cluster_len.size() < size)
          cluster_len.push_back(KMeansClustering::Length(cluster, { ignore }));
        double similarity = sqrtl(point_len * cluster_len[i]);
        similarity = KMeansClustering::Dot(cluster, point, { ignore }) / similarity;

        if (similarity >= PRECISION)
          mRecommends[i].emplace_back(index, similarity);
      }
      mTable[index] = std::move(point);
    }
  }

  double grade = 0.0;
  std::vector<int> highest;

  for (const auto recommenderList : mRecommends)
  {
    for (int i = 0; i < recommenderList.size(); ++i)
    {
      auto & obj = recommenderList[i];
      if (grade < mTable[obj.first].mDataPoints[ignore])
      {
        grade = mTable[obj.first].mDataPoints[ignore];
        highest.clear();
      }
      else if (grade == mTable[obj.first].mDataPoints[ignore])
        highest.push_back(i);
    }
  }

  highest;
}
