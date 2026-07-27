import unittest
from collections import defaultdict
from pathlib import Path


RUNNER_ROOT = Path(__file__).resolve().parents[1]
BUILDERS_DIR = RUNNER_ROOT / "src" / "generation" / "builders"
REFERENCE_MODELS_DIR = RUNNER_ROOT / "resources" / "reference-models"


def interpolate_y_sorted(sorted_series, x):
    if len(sorted_series) < 2:
        return None
    if x < sorted_series[0][0] or x > sorted_series[-1][0]:
        return None
    for i in range(1, len(sorted_series)):
        x0, y0 = sorted_series[i - 1]
        x1, y1 = sorted_series[i]
        if x0 <= x <= x1:
            if x1 == x0:
                return y1
            w = (x - x0) / (x1 - x0)
            return y0 + w * (y1 - y0)
    return None


def compute_depth_slice(rows, x_col, y_col, depth_col, target_x):
    """
    rows: list of dict-like entries with numeric keys x/y/depth
    returns list[(depth, interpolated_y)] sorted by depth
    """
    by_depth = defaultdict(list)
    for row in rows:
        by_depth[row[depth_col]].append((row[x_col], row[y_col]))

    out = []
    for depth in sorted(by_depth.keys()):
        pairs = sorted(by_depth[depth], key=lambda p: p[0])
        if target_x <= pairs[0][0]:
            out.append((depth, pairs[0][1]))
            continue
        if target_x >= pairs[-1][0]:
            out.append((depth, pairs[-1][1]))
            continue
        yi = None
        for i in range(1, len(pairs)):
            x0, y0 = pairs[i - 1]
            x1, y1 = pairs[i]
            if x0 <= target_x <= x1 and x1 != x0:
                w = (target_x - x0) / (x1 - x0)
                yi = y0 + w * (y1 - y0)
                break
        if yi is None:
            yi = pairs[0][1]
        out.append((depth, yi))
    return out


def is_known_preset(preset):
    return preset in {
        "HQ_Drywell_MonitoringWell",
        "HQ_Drywell_GroundwaterBoundary",
        "HQ_Drywell_PretreatmentChambers",
        "HQ_Drywell_SuiteStyle",
        "HQ_Drywell_LegacyStyle",
        "VN_Drywell",
        "VN_Drywell_Pro",
        "R_Bioswale_Underdrain",
        "R_Bioswale_Underdrain_GW",
        "R_Bioswale_SuiteStyle",
        "R_Bioswale_LegacyStyle",
        "JM_Bioretention_Underdrain",
        "JM_Bioretention_GW",
    }


def is_preset_compatible_with_model(preset, model_type):
    if not preset:
        return True
    hq_drywell_model = model_type.lower() in {"hq_drywell", "vn_drywell"}
    r_bioswale_model = model_type.lower() == "r_bioswale"
    jm_model = model_type.lower() == "jm_bioretention"
    hq_drywell_preset = preset.startswith("HQ_Drywell_")
    r_bioswale_preset = preset.startswith("R_Bioswale_")
    jm_preset = preset.startswith("JM_Bioretention_")
    if (hq_drywell_model and (r_bioswale_preset or jm_preset)) \
            or (r_bioswale_model and (hq_drywell_preset or jm_preset)) \
            or (jm_model and (hq_drywell_preset or r_bioswale_preset)):
        return False
    if preset in {"VN_Drywell", "VN_Drywell_Pro"} and model_type.lower() != "vn_drywell":
        return False
    return True


class TestAnalysisAlgorithms(unittest.TestCase):
    def test_interpolate_y_sorted(self):
        s = [(0.0, 0.0), (1.0, 2.0), (2.0, 4.0)]
        self.assertAlmostEqual(interpolate_y_sorted(s, 0.5), 1.0)
        self.assertAlmostEqual(interpolate_y_sorted(s, 1.5), 3.0)
        self.assertIsNone(interpolate_y_sorted(s, -1.0))
        self.assertIsNone(interpolate_y_sorted(s, 3.0))

    def test_compute_depth_slice(self):
        # Two depth levels, each with x samples at 0 and 1.
        rows = [
            {"x": 0.0, "depth": 1.0, "moisture": 0.10},
            {"x": 1.0, "depth": 1.0, "moisture": 0.30},
            {"x": 0.0, "depth": 2.0, "moisture": 0.20},
            {"x": 1.0, "depth": 2.0, "moisture": 0.60},
        ]
        out = compute_depth_slice(rows, "x", "moisture", "depth", 0.5)
        self.assertEqual(len(out), 2)
        self.assertAlmostEqual(out[0][0], 1.0)
        self.assertAlmostEqual(out[0][1], 0.20)
        self.assertAlmostEqual(out[1][0], 2.0)
        self.assertAlmostEqual(out[1][1], 0.40)

    def test_known_enrichment_presets(self):
        self.assertTrue(is_known_preset("HQ_Drywell_MonitoringWell"))
        self.assertTrue(is_known_preset("HQ_Drywell_GroundwaterBoundary"))
        self.assertTrue(is_known_preset("HQ_Drywell_PretreatmentChambers"))
        self.assertTrue(is_known_preset("HQ_Drywell_SuiteStyle"))
        self.assertTrue(is_known_preset("HQ_Drywell_LegacyStyle"))
        self.assertTrue(is_known_preset("VN_Drywell"))
        self.assertTrue(is_known_preset("VN_Drywell_Pro"))
        self.assertTrue(is_known_preset("R_Bioswale_Underdrain"))
        self.assertTrue(is_known_preset("R_Bioswale_Underdrain_GW"))
        self.assertTrue(is_known_preset("R_Bioswale_SuiteStyle"))
        self.assertTrue(is_known_preset("R_Bioswale_LegacyStyle"))
        self.assertTrue(is_known_preset("JM_Bioretention_Underdrain"))
        self.assertTrue(is_known_preset("JM_Bioretention_GW"))
        self.assertFalse(is_known_preset("HQ_Drywell_Unknown"))

    def test_preset_model_compatibility(self):
        self.assertTrue(is_preset_compatible_with_model("", "HQ_Drywell"))
        self.assertTrue(is_preset_compatible_with_model("HQ_Drywell_MonitoringWell", "HQ_Drywell"))
        self.assertFalse(is_preset_compatible_with_model("VN_Drywell", "HQ_Drywell"))
        self.assertTrue(is_preset_compatible_with_model("VN_Drywell", "VN_Drywell"))
        self.assertTrue(is_preset_compatible_with_model("VN_Drywell_Pro", "VN_Drywell"))
        self.assertTrue(is_preset_compatible_with_model("R_Bioswale_Underdrain", "R_Bioswale"))
        self.assertFalse(is_preset_compatible_with_model("R_Bioswale_Underdrain", "HQ_Drywell"))
        self.assertFalse(is_preset_compatible_with_model("HQ_Drywell_MonitoringWell", "R_Bioswale"))
        self.assertFalse(is_preset_compatible_with_model("VN_Drywell", "R_Bioswale"))
        self.assertFalse(is_preset_compatible_with_model("VN_Drywell_Pro", "R_Bioswale"))
        self.assertTrue(is_preset_compatible_with_model("JM_Bioretention_Underdrain", "JM_Bioretention"))
        self.assertTrue(is_preset_compatible_with_model("JM_Bioretention_GW", "JM_Bioretention"))
        self.assertFalse(is_preset_compatible_with_model("JM_Bioretention_Underdrain", "HQ_Drywell"))
        self.assertFalse(is_preset_compatible_with_model("JM_Bioretention_GW", "R_Bioswale"))
        self.assertFalse(is_preset_compatible_with_model("HQ_Drywell_MonitoringWell", "JM_Bioretention"))

    def test_jm_reference_ohq_contains_checked_in_topology(self):
        source = REFERENCE_MODELS_DIR / "JM.ohq"
        text = source.read_text(encoding="utf-8")
        lines = text.splitlines()

        self.assertEqual(sum(line.startswith("create block;") for line in lines), 22)
        self.assertEqual(sum(line.startswith("create link;") for line in lines), 30)
        self.assertEqual(sum(line.startswith("create parameter;") for line in lines), 4)
        self.assertEqual(text.count("type=Catchment_link"), 5)
        self.assertEqual(text.count("type=surfacewater_to_soil_link"), 4)
        self.assertEqual(text.count("type=soil_to_aggregate_link"), 4)
        self.assertEqual(text.count("type=aggregate_to_soil_link"), 4)
        self.assertEqual(text.count("type=soil_to_soil_H_link"), 3)
        self.assertEqual(text.count("type=Sewer_pipe"), 5)
        self.assertIn("name=JM DA-01", text)
        self.assertIn("name=JM Pond 1", text)
        self.assertIn("name=JM Engineered Soil 1", text)
        self.assertIn("name=JM Aggregate 1", text)
        self.assertIn("name=JM Native Soil 1-1", text)
        self.assertIn("name=JM Catch Basin", text)
        self.assertIn("name=JM Receiving Water", text)
        self.assertIn("name=JM Groundwater", text)

    def test_jm_reference_uses_r_bioswale_link_patterns(self):
        r_source = (BUILDERS_DIR / "r_bioswale_builder.cpp").read_text(encoding="utf-8")
        jm_text = (REFERENCE_MODELS_DIR / "JM.ohq").read_text(encoding="utf-8")
        shared_patterns = [
            "type=Catchment_link",
            "type=surfacewater_to_soil_link",
            "type=soil_to_soil_H_link",
            "type=aggregate_to_soil_link",
            "type=soil_to_fixedhead_link",
            "type=Sewer_pipe",
            "type=fixed_head",
        ]
        for pattern in shared_patterns:
            self.assertIn(pattern, r_source)
            self.assertIn(pattern, jm_text)

    def test_r_and_jm_reference_geometries_remain_independent(self):
        r_source = (BUILDERS_DIR / "r_bioswale_builder.cpp").read_text(encoding="utf-8")
        jm_text = (REFERENCE_MODELS_DIR / "JM.ohq").read_text(encoding="utf-8")

        self.assertIn("options.rBioSwaleWidth > 0.0 ? options.rBioSwaleWidth : 0.6096", r_source)
        self.assertIn("options.rBioSwaleDepth > 0.0 ? options.rBioSwaleDepth : 0.9144", r_source)
        self.assertIn("area=11.3032,bottom_elevation=-0.9006,depth=0.9144", jm_text)
        self.assertIn("area=11.3032,bottom_elevation=-1.8912,depth=0.9906", jm_text)
        self.assertIn("area=45.2128,bottom_elevation=-4.3362,depth=2.4", jm_text)
        self.assertIn("diameter=0.1016,end_elevation=-1.6164,length=3.048", jm_text)

    def test_registry_exposes_jm_model_type(self):
        source = RUNNER_ROOT / "src" / "generation" / "structure_registry.cpp"
        text = source.read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("JM_Bioretention")', text)
        self.assertIn('QStringLiteral("JM_Bioretention_Underdrain")', text)
        self.assertIn('QStringLiteral("JM_Bioretention_GW")', text)

    def test_qmake_project_packages_jm_reference_output(self):
        source = RUNNER_ROOT / "model_creator_runner.pro"
        text = source.read_text(encoding="utf-8")
        self.assertIn("jm_bioretention_builder.cpp", text)
        self.assertIn("jm_bioretention_builder.h", text)
        self.assertIn("DISTFILES", text)
        self.assertIn("JM.ohq", text)


if __name__ == "__main__":
    unittest.main()
