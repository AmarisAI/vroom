/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "output_json.h"

rapidjson::Document to_json(const solution& sol, bool geometry) {
  rapidjson::Document json_output;
  json_output.SetObject();
  rapidjson::Document::AllocatorType& allocator = json_output.GetAllocator();

  json_output.AddMember("code", sol.code, allocator);
  if (sol.code != 0) {
    json_output.AddMember("error", rapidjson::Value(), allocator);
    json_output["error"].SetString(sol.error.c_str(), sol.error.size());
  } else {
    json_output.AddMember("summary",
                          to_json(sol.summary, geometry, allocator),
                          allocator);

    rapidjson::Value json_unassigned(rapidjson::kArrayType);
    for (const auto& job : sol.unassigned) {
      rapidjson::Value json_job(rapidjson::kObjectType);
      json_job.AddMember("id", job.id, allocator);
      if (job.has_coordinates()) {
        json_job.AddMember("location", to_json(job, allocator), allocator);
      }
      json_unassigned.PushBack(json_job, allocator);
    }

    json_output.AddMember("unassigned", json_unassigned, allocator);

    rapidjson::Value json_routes(rapidjson::kArrayType);
    for (const auto& route : sol.routes) {
      json_routes.PushBack(to_json(route, allocator), allocator);
    }

    json_output.AddMember("routes", json_routes, allocator);
  }

  return json_output;
}

rapidjson::Value to_json(const summary_t& summary,
                         bool geometry,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value json_summary(rapidjson::kObjectType);

  json_summary.AddMember("cost", summary.cost, allocator);
  json_summary.AddMember("unassigned", summary.unassigned, allocator);

  if (geometry) {
    json_summary.AddMember("distance", summary.distance, allocator);
    json_summary.AddMember("duration", summary.duration, allocator);
  }

  json_summary.AddMember("computing_times",
                         to_json(summary.computing_times, geometry, allocator),
                         allocator);

  return json_summary;
}

rapidjson::Value to_json(const route_t& route,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value json_route(rapidjson::kObjectType);

  json_route.AddMember("vehicle", route.vehicle, allocator);
  json_route.AddMember("cost", route.cost, allocator);

  if (!route.geometry.empty()) {
    json_route.AddMember("distance", route.distance, allocator);
    json_route.AddMember("duration", route.duration, allocator);
  }

  rapidjson::Value json_steps(rapidjson::kArrayType);
  for (const auto& step : route.steps) {
    json_steps.PushBack(to_json(step, allocator), allocator);
  }

  json_route.AddMember("steps", json_steps, allocator);

  if (!route.geometry.empty()) {
    json_route.AddMember("geometry", rapidjson::Value(), allocator);
    json_route["geometry"].SetString(route.geometry.c_str(),
                                     route.geometry.size());
  }

  return json_route;
}

rapidjson::Value to_json(const computing_times_t& ct,
                         bool geometry,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value json_ct(rapidjson::kObjectType);

  json_ct.AddMember("loading", ct.loading, allocator);
  json_ct.AddMember("solving", ct.solving, allocator);

  if (geometry) {
    // Log route information timing when using OSRM.
    json_ct.AddMember("routing", ct.routing, allocator);
  }

  return json_ct;
}

rapidjson::Value to_json(const step& s,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value json_step(rapidjson::kObjectType);

  json_step.AddMember("type", rapidjson::Value(), allocator);
  std::string str_type;
  switch (s.type) {
  case TYPE::START:
    str_type = "start";
    break;
  case TYPE::END:
    str_type = "end";
    break;
  case TYPE::JOB:
    str_type = "job";
    break;
  }
  json_step["type"].SetString(str_type.c_str(), str_type.size(), allocator);

  if (s.location.has_coordinates()) {
    json_step.AddMember("location", to_json(s.location, allocator), allocator);
  }

  if (s.type == TYPE::JOB) {
    json_step.AddMember("job", s.job, allocator);
  }

  return json_step;
}

rapidjson::Value to_json(const location_t& loc,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value json_coords(rapidjson::kArrayType);

  json_coords.PushBack(loc.lon(), allocator);
  json_coords.PushBack(loc.lat(), allocator);

  return json_coords;
}

void write_to_json(const solution& sol,
                   bool geometry,
                   const std::string& output_file) {
  // Set output.
  auto start_output = std::chrono::high_resolution_clock::now();
  std::string out = output_file.empty() ? "standard output" : output_file;
  BOOST_LOG_TRIVIAL(info) << "[Output] Write solution to " << out << ".";

  auto json_output = to_json(sol, geometry);

  // Rapidjson writing process.
  rapidjson::StringBuffer s;
  rapidjson::Writer<rapidjson::StringBuffer> r_writer(s);
  json_output.Accept(r_writer);

  // Write to relevant output.
  if (output_file.empty()) {
    // Log to standard output.
    std::cout << s.GetString() << std::endl;
  } else {
    // Log to file.
    std::ofstream out_stream(output_file, std::ofstream::out);
    out_stream << s.GetString();
    out_stream.close();
  }
  auto end_output = std::chrono::high_resolution_clock::now();
  auto output_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                           end_output - start_output)
                           .count();

  BOOST_LOG_TRIVIAL(info) << "[Output] Done, took " << output_duration
                          << " ms.";
}
