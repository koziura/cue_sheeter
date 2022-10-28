#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <fmt/format.h>

#include <FLAC/format.h>
#include <FLAC/metadata.h>

namespace fs = std::filesystem;

struct cvt_toupper {
	char operator()(const std::string::value_type& v) const {
		return static_cast<std::string::value_type>(std::toupper(v));
	}
};
struct cvt_tolower {
	char operator()(const std::string::value_type& v) const {
		return static_cast<std::string::value_type>(std::tolower(v));
	}
};
template <typename T>
inline std::string string_to_copy(const std::string& source) {
	static_assert(
		std::is_same<T, cvt_toupper>::value ||
		std::is_same<T, cvt_tolower>::value,
		"type must be cvt_toupper or cvt_tolower"
	);
	std::string result;
	result.reserve(source.size());
	std::transform(source.begin(), source.end(), std::back_inserter(result), T());
	return result;
}

std::string tag_parser(bool& ok, std::string_view source, std::string_view tag, std::string_view remove = {}) {
	std::string value;
	auto idx = source.find(tag);
	if (idx != std::string::npos) {
		value = source.substr(idx + tag.length() + 1);
		if (!remove.empty()) {
			value.erase(std::remove(value.begin(), value.end(), remove.front()), value.end());
		}
		ok = true;
	} else {
		ok = false;
	}
	return value;
}

int main(int argc, char *argv[]) {
	std::string album_artist;
	std::string album_title;
	std::string album_date;
	std::string album_genre;

	std::vector<std::string> header_vals;
	std::vector<std::string> track_vals;

	fs::path path;
	if (argc > 1) {
		path = fs::absolute(argv[1]);
	} else {
		path = fs::absolute("cue_template.txt");
	}

	std::ifstream ifs;
	if (path.filename().string() == "cue_template.txt") {
		ifs.open(path);
	}

	if (ifs.is_open()) {
		std::string line;
		while (std::getline(ifs, line)) {
			bool ok{};

			if (album_genre.empty() && !ok) {
				fmt::print(stdout, "find tag GENRE: ");
				auto result = tag_parser(ok, line, "GENRE", "\"");
				if (ok) {
					album_genre = std::move(result);
				} else {
					fmt::print(stdout, "none\n");
				}
			}
			if (album_artist.empty() && !ok) {
				fmt::print(stdout, "find tag PERFORMER: ");
				auto result = tag_parser(ok, line, "PERFORMER", "\"");
				if (ok) {
					album_artist = std::move(result);
				} else {
					fmt::print(stdout, "none\n");
				}
			}
			if (album_date.empty() && !ok) {
				fmt::print(stdout, "find tag DATE: ");
				auto result = tag_parser(ok, line, "DATE");
				if (ok) {
					album_date = std::move(result);
				} else {
					fmt::print(stdout, "none\n");
				}
			}
			if (album_title.empty() && !ok) {
				fmt::print(stdout, "find tag TITLE: ");
				auto result = tag_parser(ok, line, "TITLE", "\"");
				if (ok) {
					album_title = std::move(result);
				} else {
					fmt::print(stdout, "none\n");
				}
			}

			fmt::print(stdout, "{}\n", line);
			header_vals.emplace_back(std::move(line) + "\n");
		}
		ifs.close();
	} else {
		fmt::print(stdout, "can't open 'cue_template.txt'\n");
	}

	int track{1};
	std::set<fs::path> tracks;
	const auto track_mask = "FILE \"{}\" WAVE\n  TRACK {:02} AUDIO\n\tTITLE {}\n\tINDEX 01 00:00:00\n";
	auto parent_path = path.parent_path().string();
	std::error_code ec;
	for (const auto& entry : fs::directory_iterator(parent_path, ec)) {
		const auto& path = entry.path();
		auto extension = string_to_copy<cvt_tolower>(path.extension().string());
		if (extension == ".flac") {
			tracks.emplace(path);
		}
	}

	for (const auto& path : tracks) {
		auto filename = path.filename().string();
		auto track_title = path.stem().string();

		FLAC__StreamMetadata* tags{};
		FLAC__metadata_get_tags(path.string().c_str(), &tags);
		if (tags) {
			fmt::print(stdout, "track {} tag:\n", track);
			for (uint32_t i = 0; i < tags->data.vorbis_comment.num_comments; ++i) {
				const auto& length = tags->data.vorbis_comment.comments[i].length;
				std::string comment;
				comment.resize(length);
				memcpy(comment.data(), tags->data.vorbis_comment.comments[i].entry, length);

				bool ok{};
				auto result = tag_parser(ok, comment, "TITLE");
				if (ok) {
					track_title = std::move(result);
				}

				if (album_artist.empty() && !ok) {
					auto result = tag_parser(ok, comment, "ARTIST");
					if (ok) {
						album_artist = std::move(result);
					}
				}

				if (album_title.empty() && !ok) {
					auto result = tag_parser(ok, comment, "ALBUM");
					if (ok) {
						album_title = std::move(result);
					}
				}

				if (album_date.empty() && !ok) {
					auto result = tag_parser(ok, comment, "DATE");
					if (ok) {
						album_date = std::move(result);
					}
				}

				if (album_genre.empty() && !ok) {
					auto result = tag_parser(ok, comment, "GENRE");
					if (ok) {
						album_genre = std::move(result);
					}
				}

				fmt::print(stdout, "\t{}\n", comment);
			}
		}

		track_vals.emplace_back(fmt::format(track_mask, filename, track, track_title));
		++track;
	}

	if (track > 1) {
		if (header_vals.empty()) {
			header_vals.emplace_back(fmt::format("REM GENRE \"{}\"\n", album_genre));
			header_vals.emplace_back(fmt::format("REM DATE {}\n", album_date));
			header_vals.emplace_back(fmt::format("PERFORMER \"{}\"\n", album_artist));
			header_vals.emplace_back(fmt::format("TITLE \"{}\"\n", album_title));
		}

		auto output_cue = fmt::format("{}/{} - {}. {}.cue", parent_path, album_artist, album_date, album_title);
		std::ofstream ofs(output_cue, std::ios::trunc | std::ios::ate);
		if (ofs.is_open()) {
			for (const auto& value : header_vals) {
				ofs.write(value.data(), value.size());
				fmt::print(stdout, "{}\n", value);
			}

			for (const auto& value : track_vals) {
				ofs.write(value.data(), value.size());
				fmt::print(stdout, "{}\n", value);
			}
			ofs.close();
		}
	} else {
		if (ec) {
			fmt::print(stdout, "{}\n", ec.message());
		}
		fmt::print(stdout, "tracks not found!\n");
	}

	return 0;
}
