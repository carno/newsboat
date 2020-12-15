#ifndef PODBOAT_QUEUELOADER_H_
#define PODBOAT_QUEUELOADER_H_

#include <functional>
#include <vector>

#include "3rd-party/optional.hpp"

#include "configcontainer.h"
#include "download.h"

namespace podboat {

class QueueLoader {
public:
	QueueLoader(const std::string& file, newsboat::ConfigContainer& cfg,
		std::function<void()> cb_require_view_update);
	void reload(std::vector<Download>& downloads,
		bool also_remove_finished = false) const;

private:
	std::string get_filename(const std::string& str) const;

	std::string queuefile;
	newsboat::ConfigContainer& cfg;
	std::function<void()> cb_require_view_update;

	/// A helper type for methods that process the queue file.
	struct CategorizedDownloads {
		/// Downloads that should be kept in the queue file.
		std::vector<Download> to_keep;

		/// Downloads that should be deleted from the queue file.
		std::vector<Download> to_delete;
	};

	/// Splits downloads into "to keep" and "to delete" categories depending on
	/// their status.
	///
	/// If `also_remove_finished` is `true`, downloads with `FINISHED` status
	/// are put into "to delete" category.
	///
	/// Returns:
	/// - nullopt if one of the downloads is currently being downloaded;
	/// - otherwise, a struct with categorized downloads.
	static nonstd::optional<CategorizedDownloads> categorize_downloads(
		const std::vector<Download>& downloads, bool also_remove_finished);

	/// Adds downloads from the queue file to the "to keep" category.
	void update_from_queue_file(CategorizedDownloads& downloads) const;

	/// Writes "to keep" downloads to the queue file.
	void write_queue_file(const CategorizedDownloads& downloads) const;

	/// If `delete-played-files` is enabled, deletes downloaded files
	/// corresponding to downloads in the "to delete" category.
	void delete_played_files(const CategorizedDownloads& downloads) const;
};

} // namespace podboat

#endif /* PODBOAT_QUEUELOADER_H_ */
