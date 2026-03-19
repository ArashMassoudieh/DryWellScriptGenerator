import unittest
from collections import defaultdict


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
        "Drywell_MonitoringWell",
        "Drywell_GroundwaterBoundary",
        "Drywell_PretreatmentChambers",
        "Bioswale_Underdrain",
        "Bioswale_Underdrain_GW",
    }


def is_preset_compatible_with_model(preset, model_type):
    if not preset:
        return True
    drywell_model = model_type.lower() == "drywell"
    bioswale_model = model_type.lower() == "bioswale"
    drywell_preset = preset.startswith("Drywell_")
    bioswale_preset = preset.startswith("Bioswale_")
    if (drywell_model and bioswale_preset) or (bioswale_model and drywell_preset):
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
        self.assertTrue(is_known_preset("Drywell_MonitoringWell"))
        self.assertTrue(is_known_preset("Drywell_GroundwaterBoundary"))
        self.assertTrue(is_known_preset("Drywell_PretreatmentChambers"))
        self.assertTrue(is_known_preset("Bioswale_Underdrain"))
        self.assertTrue(is_known_preset("Bioswale_Underdrain_GW"))
        self.assertFalse(is_known_preset("Drywell_Unknown"))

    def test_preset_model_compatibility(self):
        self.assertTrue(is_preset_compatible_with_model("", "Drywell"))
        self.assertTrue(is_preset_compatible_with_model("Drywell_MonitoringWell", "Drywell"))
        self.assertTrue(is_preset_compatible_with_model("Bioswale_Underdrain", "Bioswale"))
        self.assertFalse(is_preset_compatible_with_model("Bioswale_Underdrain", "Drywell"))
        self.assertFalse(is_preset_compatible_with_model("Drywell_MonitoringWell", "Bioswale"))


if __name__ == "__main__":
    unittest.main()
