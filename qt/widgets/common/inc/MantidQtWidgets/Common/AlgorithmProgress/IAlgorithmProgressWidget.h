#pragma once
class QString;

namespace MantidQt {
namespace MantidWidgets {
class IAlgorithmProgressWidget {
public:
  IAlgorithmProgressWidget() = default;
  ~IAlgorithmProgressWidget() = default;

  virtual void algorithmStarted() = 0;
  virtual void algorithmEnded() = 0;
  virtual void updateProgress(double progress, const QString &message) = 0;
  virtual void showDetailsDialog() = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
