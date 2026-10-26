# Compatibility database v1
games.json is an extensible JSON object with schema_version and games array.
Each game records name, regional title_identifier (null until verified), game_version, requested/tested ps5_firmware, headset, required_features and required_features_verified, launch/rendering/tracking/controller status, known_issues, evidence and test_date.
Status values: not_tested, blocked, partial, working, failed. "working" requires dated evidence identifying the tested configuration. Requested firmware on an untested entry does not constitute a tested version.
Evidence should contain source/log references and observation summaries. No proprietary assets.
13.20 firmware descriptor is a requested target, not runtime detection or verified compatibility. Native code returns disabled capabilities for all inputs until a platform implementation has evidence.
