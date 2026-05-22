#include "ExtEditorDisplayView.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocTypes.h"

#include "imgui.h"

#include <cstddef>
#include <cstdio>

namespace app::display {
namespace {

const char* severityLabel(app::doc::DiagnosticSeverity severity) {
  switch (severity) {
  case app::doc::DiagnosticSeverity::Error:
    return "error";
  case app::doc::DiagnosticSeverity::Warning:
    return "warning";
  case app::doc::DiagnosticSeverity::Info:
    return "info";
  }
  return "unknown";
}

const char* applyStatusLabel(app::doc::ApplyStatus status) {
  switch (status) {
  case app::doc::ApplyStatus::Idle:
    return "idle";
  case app::doc::ApplyStatus::Validated:
    return "validated";
  case app::doc::ApplyStatus::Planned:
    return "planned";
  case app::doc::ApplyStatus::Admitted:
    return "admitted";
  case app::doc::ApplyStatus::Started:
    return "applying";
  case app::doc::ApplyStatus::Completed:
    return "applied";
  case app::doc::ApplyStatus::Failed:
    return "failed";
  case app::doc::ApplyStatus::Superseded:
    return "superseded";
  }
  return "unknown";
}

void countDiagnostics(const app::doc::DocDiagnostics& diagnostics,
                      uint32_t* errors,
                      uint32_t* warnings) {
  *errors = 0;
  *warnings = 0;

  for (const app::doc::DocDiagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == app::doc::DiagnosticSeverity::Error)
      ++(*errors);
    else if (diagnostic.severity == app::doc::DiagnosticSeverity::Warning)
      ++(*warnings);
  }
}

void drawDiagnosticList(const app::doc::DocDiagnostics& diagnostics) {
  if (diagnostics.empty()) {
    ImGui::TextDisabled("No diagnostics");
    return;
  }

  for (std::size_t i = 0; i < diagnostics.size(); ++i) {
    const app::doc::DocDiagnostic& diagnostic = diagnostics[i];
    ImGui::PushID(static_cast<int>(i));

    ImGui::Text("%s line %u:%u",
                severityLabel(diagnostic.severity),
                diagnostic.span.line,
                diagnostic.span.column);
    ImGui::SameLine();
    ImGui::TextUnformatted(diagnostic.message.c_str());

    if (!diagnostic.relatedTarget.empty())
      ImGui::TextDisabled("  %s", diagnostic.relatedTarget.c_str());

    ImGui::PopID();
  }
}

} // namespace

void drawExtEditorDisplayView(AppContext& app) {
  const app::doc::DocAuthoringService& service = app.documents.authoring;
  const app::doc::DocDiagnostics& diagnostics = app::doc::getDocDiagnostics(service);
  const app::doc::ApplyStatus status = app::doc::getApplyStatus(service);

  ImGui::Spacing();
  ImGui::Text("Apply status: %s", applyStatusLabel(status));

  uint32_t errors = 0;
  uint32_t warnings = 0;
  countDiagnostics(diagnostics, &errors, &warnings);

  char header[96];
  std::snprintf(header,
                sizeof(header),
                "Diagnostics (%u errors, %u warnings)###ExternalEditorDiagnostics",
                errors,
                warnings);

  ImGui::Spacing();
  if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::BeginChild("ExternalEditorDiagnosticsList",
                      ImVec2(0.0f, 320.0f),
                      ImGuiChildFlags_Borders);
    drawDiagnosticList(diagnostics);
    ImGui::EndChild();
  }
}

} // namespace app::display
