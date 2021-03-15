/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/impl/fallback_configurator.hpp>
#include <soralog/macro.hpp>

#include "logging_object.hpp"

enum ConfiguratorType {
  Fallback,
  Customized,
  YamlByPath,
  YamlByContent,
  Cascade
};

std::shared_ptr<soralog::Configurator> customized_configurator = [] {
  static auto cfg = std::make_shared<soralog::FallbackConfigurator>();
  cfg->setLevel(soralog::Level::TRACE);
  cfg->withColor(true);
  return cfg;
}();

std::shared_ptr<soralog::Configurator> yaml_configurator_from_file =
    std::make_shared<soralog::ConfiguratorFromYAML>(
        std::filesystem::path("../../../example/01-simple/logger.yml"));

std::shared_ptr<soralog::Configurator> yaml_configurator_by_content =
    std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main_
    sink: console
    level: trace
  - name: azaza
  )"));

std::shared_ptr<soralog::Configurator> cascade_configurator = [] {
  auto prev = std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
groups:
  - name: main
    level: info
    children:
      - name: first-1
        children:
          - name: second-1-1
          - name: second-1-2
            children:
              - name: third-1-2-1
          - name: second-1-3
      - name: first-2
        children:
          - name: second-2-1
          - name: second-2-2
      - name: first-3
  )"));

  return std::make_shared<soralog::ConfiguratorFromYAML>(std::move(prev),
                                                         std::string(R"(
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main
    sink: console
    level: trace
  )"));
}();

int main() {
  ConfiguratorType cfg_type = ConfiguratorType::Cascade;

  std::shared_ptr<soralog::Configurator> configurator =
      cfg_type == ConfiguratorType::Cascade
      ? cascade_configurator
      : cfg_type == ConfiguratorType::YamlByContent
          ? yaml_configurator_by_content
          : cfg_type == ConfiguratorType::YamlByPath
              ? yaml_configurator_from_file
              : cfg_type == ConfiguratorType::Customized
                  ? customized_configurator
                  : std::make_shared<soralog::FallbackConfigurator>();

  soralog::LoggingSystem log_system(configurator);

  auto r = log_system.configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  auto main_log = log_system.getLogger("main", "*");

  main_log->info("Start");

  auto lambda = [](const auto &tag) {
    std::cout << "CALCULATED AT LOGGING: " << tag << std::endl;
    return "message";
  };

  main_log->debug("Debug: {}", lambda("logger: debug for trace level"));
  SL_DEBUG(main_log, "Debug: {}", lambda("macro: debug for trace level"));

  main_log->setLevel(soralog::Level::INFO);

  main_log->trace("Debug: {}", lambda("logger: debug for info level"));
  SL_DEBUG(main_log, "Debug: {}", lambda("macro: debug for info level"));

  LoggingObject object(log_system);

  object.method();

  main_log->info("Finish");

  return 0;
}
