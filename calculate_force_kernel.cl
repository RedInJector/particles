
float distancePow2(const float2 *p1, const float2 *p2) {
    return pow(p1->x - p2->x, 2) + pow(p1->y - p2->y, 2);
}
float distance(const float2 *p1, const float2 *p2) {
    return sqrt(distancePow2(p1, p2));
}

float2 calculate_force_gpu(float2 *p1, float2 *p2, float mass){
    float2 force_vector;
    float dist = distance(p1, p2);

    if (dist < 0.0000001){
        force_vector.x = 0;
        force_vector.y = 0;
        return force_vector;    
    }
    float force = 10 / dist * dist;
    float dx = p1->x - p2->x;
    float dy = p1->y - p2->y;

    force_vector.x = force * dx / dist;
    force_vector.y = force * dy / dist;

    return force_vector;
}


__kernel void calculate_force(
        __global float2 *tiles,
        __global float *masses,
        int max_col,
        int max_row,
        __global float2 *out_forces
    ){
    int i = get_global_id(0);
    int j = get_global_id(1);

    for (int k = 0; k < max_col; k++) {
        for (int n = 0; n < max_row; n++) {
            float2 forcev = calculate_force_gpu(
                &tiles[k * max_col + n], 
                &tiles[i * max_col + j], 
                masses[i * max_col + j] + masses[k * max_col + n]);
            *out_forces += forcev;
        }
    }
}
