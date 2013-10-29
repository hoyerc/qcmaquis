#ifndef PLACEMENT_H
#define PLACEMENT_H

inline void make_consistent(std::vector<int>& p_new, int b2, std::vector<int>& e_new,
                            const std::vector<int>& p_old, int b1, std::vector<int>& e_old)
{
    // exceptions check
    for(int i = 0; i < e_old.size(); i++) if(b1 == e_old[i]) return;
    for(int i = 0; i < e_new.size(); i++) if(b2 == e_new[i]) return;
    // common check
    for(int i = 0; i < p_new.size(); i++){
        if(p_new[i] == p_old[b1]){
            e_old.push_back(b1);
            for(int ii = i; ii < p_new.size(); ii++) 
            if(p_new[ii] == p_old[b1]) p_new[ii] = -1;
            return;
        }
    }
    // overlap check
    if(p_new[b2] != -1){
        e_new.push_back(b2);
        p_new[b2] = -1;
        return;
    }

    p_new[b2] = p_old[b1];
}

inline void make_complete(std::vector<int>& placement){
    for(int i = 0; i < placement.size(); i++)
    if(placement[i] == -1){
        for(int k = 0; k < placement.size(); k++){
            bool found = false;
            for(int kk = 0; kk < placement.size(); kk++) if(k == placement[kk]){ found = true; break; }
            if(found) continue;
            placement[i] = k;
            break;
        }
    }
}

template<class Matrix, class SymmGroup>
std::vector<int> get_left_placement(const MPOTensor<Matrix, SymmGroup>& mpo, const std::vector<int>& placement_l_old, const std::vector<int>& placement_r){
    std::vector<int> ts_exceptions_l, ts_exceptions_r;
    std::vector<int> placement_l(placement_l_old.size(), -1);
    
    for(int b1 = 0; b1 < placement_l_old.size(); b1++)
    for(int b2 = 0; b2 < placement_r.size(); b2++){
        if(mpo.has(b1,b2)){
            make_consistent(placement_l, b1, ts_exceptions_l, 
                            placement_r, b2, ts_exceptions_r);
        }
    }
    make_complete(placement_l);
    return placement_l;
}

template<class Matrix, class SymmGroup>
std::vector<int> get_right_placement(const MPOTensor<Matrix, SymmGroup>& mpo, const std::vector<int>& placement_l, const std::vector<int>& placement_r_old){
    std::vector<int> ts_exceptions_l, ts_exceptions_r;
    std::vector<int> placement_r(placement_r_old.size(), -1);

    for(int b1 = 0; b1 < placement_l.size(); b1++)
    for(int b2 = 0; b2 < placement_r_old.size(); b2++){
        if(mpo.has(b1,b2)){
            make_consistent(placement_r, b2, ts_exceptions_r, 
                            placement_l, b1, ts_exceptions_l);
        }
    }
    make_complete(placement_r);
    return placement_r;
}

template<class Matrix, class SymmGroup>
std::vector<std::vector<int> > construct_placements(const MPO<Matrix, SymmGroup>& mpo){
    std::vector<std::vector<int> > placements(mpo.length()+1);
    std::vector<std::pair<std::vector<int>, std::vector<int> > > exceptions(mpo.length()+1);
    placements[0].push_back(0); // left_[0] has only 1 element
    for(int s = 0; s < mpo.length(); s++){
        placements[s+1].resize(mpo[s].col_dim(), -1);

        for(int b1 = 0; b1 < placements[s].size(); b1++)
        for(int b2 = 0; b2 < placements[s+1].size(); b2++){
            if(mpo[s].has(b1,b2)){
                make_consistent(placements[s+1], b2, exceptions[s+1].second, placements[s], b1, exceptions[s].first);
            }
        }
        make_complete(placements[s+1]);
        mpo[s].placement_l = placements[s];
        mpo[s].placement_r = placements[s+1];
    }
    for(int s = 0; s < mpo.length()+1; s++){
        maquis::cout << "Left " << s << ": ";
        maquis::cout << "E1: "; for(int k = 0; k < exceptions[s].first.size(); k++) maquis::cout << exceptions[s].first[k] << " "; 
        maquis::cout << "; E2: "; for(int k = 0; k < exceptions[s].second.size(); k++) maquis::cout << exceptions[s].second[k] << " ";
        maquis::cout << "; PL: "; for(int k = 0; k < placements[s].size(); k++) maquis::cout << placements[s][k] << " ";
        maquis::cout << "\n";
    }
    return placements;
}

#endif


