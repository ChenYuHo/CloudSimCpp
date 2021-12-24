import json
import sys
import torch
import torch.distributed as dist
from statistics import median

if len(sys.argv) < 2:
    print("Usage: python get_model_size_and_fp_bp_median.py TRACE_FILE_PATH [BUCKET_SIZE_MB]")
    sys.exit(1)

with open(sys.argv[1]) as f:

    trace = json.load(f)

    model_num_params = [trace['layer_costs'][key]['weights_bytes']//4 for key in trace['layer_costs']]
    fp_times_ps = [int(median(trace['layer_costs'][key]['forward_pass_units'])*1000) for key in trace['layer_costs']]
    bp_times_ps = [int(median(trace['layer_costs'][key]['backward_pass_units'])*1000) for key in trace['layer_costs']]
    if len(sys.argv) > 2:
        indices, _ = dist._compute_bucket_assignment_by_size([torch.empty([i]) for i in model_num_params], [1024*1024, int(sys.argv[2])*1024*1024])
        model_num_params = [sum([model_num_params[i] for i in idx]) for idx in indices]
        fp_times_ps = [sum([fp_times_ps[i] for i in idx]) for idx in indices]
        bp_times_ps = [sum([bp_times_ps[i] for i in idx]) for idx in indices]

    print(f'''model = vector<uint64_t>{{{', '.join([str(m) for m in model_num_params])}}};
forward_pass_time = vector<uint64_t>{{{', '.join([str(f) for f in fp_times_ps])}}};
backward_pass_time = vector<uint64_t>{{{', '.join([str(b) for b in bp_times_ps])}}};''')

