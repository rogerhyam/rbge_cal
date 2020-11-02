<?php

// display a tree of venues with an indication of who
// the managers are
function rbge_cal_venue_managers(){
    
    module_load_include('inc', 'rbge_cal', 'includes/venue_event_tree');
    $venue_tree = get_venue_event_tree(null);
    
    
    $roots = $venue_tree[-1];
    unset($venue_tree[-1]);
    
    $out = "<ul>";
    foreach($roots as $root){
        $out .= "<li>";
        rbge_cal_render_venue_list($out, $root, array());
        $out .= "</li>";
    }
    $out .= "</ul>";
    
    return $out;
    
    
}

function rbge_cal_render_venue_list(&$out, $venue, $ancestor_managers){
    
    $out .= "<a href=\"/taxonomy/term/$venue->tid\">$venue->name</a>";

    $out .= ' <strong>Managers: </strong>';
    
    // does it have a manager?
    $local_managers = array();
    $out_array = array();
    if(isset($venue->field_venue_manager['und']) && count($venue->field_venue_manager['und']) > 0){
        foreach($venue->field_venue_manager['und'] as $user_id){
            $user = entity_load_single('user', $user_id['target_id']);
            $local_managers[] = $user;
            $out_array[] = "<a href=\"mailto:$user->mail\">$user->name</a>";
        }
        $out .= implode(', ', $out_array);
    }
    
    // write out the inherited managers
    $out_array = array();
    if(count($ancestor_managers) > 0){
        foreach($ancestor_managers as $user){
            $out_array[] = "<a href=\"mailto:$user->mail\">$user->name</a>";
        }
        $out .= " + ";
        $out .= implode(', ', $out_array);
    }
    
    $legacy_managers = array_merge($ancestor_managers, $local_managers);
    
    if(isset($venue->children) && count($venue->children) > 0){
        $out .= "<ul>";
        foreach($venue->children as $child){
            $out .= "<li>";
            rbge_cal_render_venue_list($out, $child, $legacy_managers);
            $out .= "</li>";
        }
        $out .= "</ul>";
    } 
    
}



?>