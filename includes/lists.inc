<?php

function rbge_cal_lists(){
    
    return "<p>Useful lists of events</p>";
    
}

function rbge_cal_lists_pending(){
    
  
    // get a list of the events that are pending.
    $event_nids = array();
    
    $sql = "SELECT ed.entity_id as id
    FROM `field_data_field_event_date` as ed
    JOIN `field_data_field_venue_approval_requested` as var ON ed.entity_id = var.entity_id
    JOIN `field_data_field_cat_approval_requested` as car ON ed.entity_id = car.entity_id
    JOIN `field_data_field_category_manager_approved` as ca on ed.entity_id = ca.entity_id
    JOIN `field_data_field_venue_manager_approved` as va on ed.entity_id = va.entity_id
    WHERE
    	CAST(ed.`field_event_date_value` AS DATETIME) > now()
    AND
    	(
    		(var.`field_venue_approval_requested_value` = 1 AND va.`field_venue_manager_approved_value` = 0)
    		OR
    		(car.`field_cat_approval_requested_value` = 1 AND ca.`field_category_manager_approved_value` = 0)
    	)
    group by ed.entity_id
    order by CAST(ed.`field_event_date_value` AS DATETIME) desc";
    
    $result = db_query($sql);
    
    foreach($result as $hit){         
       $event_nids[] = $hit->id;
    }
    
    // write it out 
    $out = "<p>Events awaiting approval.</p>";
    $out .= rbge_cal_lists_header();
    
    foreach($event_nids as $nid){
          $event = entity_metadata_wrapper('node', node_load($nid));
          $out .= rbge_cal_lists_row($event);
    }
    
    $out .= rbge_cal_lists_footer();
    
    return $out;
    
}

function rbge_cal_lists_my_events(){
    
    global $user;
    $uid = $user->uid;
    
    // get the last 100 events we have edited
    $sql = "SELECT nid from node_revision
    where uid = $uid
    group by nid
    order by `timestamp` DESC
    limit 100";
    
    $result = db_query($sql);
    
    foreach($result as $hit){         
       $event_nids[] = $hit->nid;
    }
    
    // write it out 
    $out = "<p>A list of the last 100 events you have edited.</p>";
    $out .= rbge_cal_lists_header();

    foreach($event_nids as $nid){
         $event = entity_metadata_wrapper('node', node_load($nid));
         if($event->type->value() != 'event') continue;
         $out .= rbge_cal_lists_row($event);
    }

    $out .= rbge_cal_lists_footer();

    return $out;
    
}

function rbge_cal_lists_by_category(){
    
    // build a select for the categories
    $cat_voc = taxonomy_vocabulary_machine_name_load('event_category');
    $cats = taxonomy_get_tree($cat_voc->vid);
    // convert to an indexed array
    $cat_tree = array();
    foreach($cats as $cat){
        $cat_tree[$cat->tid] = $cat;
    }
    
    $pick_list = array();
    foreach($cat_tree as $cat){
        $pick_list[$cat->tid] = rbge_cal_lists_build_cat_path($cat, $cat_tree, '');
    }
    
    $current_tid = @$_GET['tid'];
    if($current_tid){
        // we have a tid from the get so put it in the session so it is sticky
        $_SESSION['rbge_cal_list_tid'] = $current_tid;
    }else{
        
        // not got it from call so try session 
        $current_tid = @$_SESSION['rbge_cal_list_tid'];
        
        // if we still don't have it use the first on the list
        if(!$current_tid) $current_tid = array_keys($pick_list)[0];
        
    }
  
    // build the select
    $out = '<form method="GET">';
    $out .= 'A list of the next events in the <select name="tid" onchange="this.form.submit()">';
    foreach($pick_list as $tid => $label){
        
        if($tid == $current_tid) $selected = 'selected';
        else $selected = '';
        
        $out .= "<option $selected value=\"$tid\">$label</option>";
    }
    $out .= "</select> category (max 100). This is not hierarchical.";
    $out .= "</form>";

    $sql = "SELECT cat.entity_id as nid
    from field_data_field_event_category as cat
    join field_data_field_event_date as d on d.entity_id = cat.entity_id
    where field_event_category_tid = $current_tid
    and CAST(d.field_event_date_value AS DATETIME) > now()
    group by cat.entity_id
    order by CAST(d.field_event_date_value AS DATETIME)
    limit 100";
    
    $result = db_query($sql);
    
    $event_nids = array();
    foreach($result as $hit){         
       $event_nids[] = $hit->nid;
    }
    
    if($event_nids){
        $out .= rbge_cal_lists_header();
        foreach($event_nids as $nid){
             $event = entity_metadata_wrapper('node', node_load($nid));
             if($event->type->value() != 'event') continue;
             $out .= rbge_cal_lists_row($event);
        }
        $out .= rbge_cal_lists_footer();
    }else{
        $out .= "No events coming up in the <strong>".  $pick_list[$current_tid]  ."</strong> category.";
    }

    return $out;
}

function rbge_cal_lists_build_cat_path($cat, $cats, $path){
    
    if($path) $path = " > " . $path;
    
    $path = $cat->name . $path;
    if($cat->parents[0] != 0){
        return rbge_cal_lists_build_cat_path($cats[$cat->parents[0]], $cats, $path);
    }else{
        return $path;
    }
    
}

function rbge_cal_lists_row($event){
    
    global $rbge_cal_lists_row_even;
    
    if($rbge_cal_lists_row_even){
        $out = '<tr class="rbge-cal-even">';
        $rbge_cal_lists_row_even = false;
    }else{
        $out = '<tr class="rbge-cal-odd">';
        $rbge_cal_lists_row_even = true;
    }

    $out .= "<td>";
    $out .= $event->getIdentifier();
    $out .= "</td>";
    
    $out .= "<td>";
    $out .= '<a href="/node/'. $event->getIdentifier() .'" >';
    $out .= $event->title->value();
    $out .= "</a>";
    $out .= "</td>";
    
    $repeat_count = count($event->field_event_date);
    $first_repeat = $event->field_event_date[0]->value();
    $start = new DateTime($first_repeat['value']);
    
    $out .= '<td>'. $start->format('D');
    $out .= '&nbsp;'. $start->format('d');
    $out .= '&nbsp;'. $start->format('M') . '</td>';
    $out .= '<td>'. $repeat_count .'</td>';
    
    // venue
    $venue_managers = array();
    $out .= "<td>";
    $venue = $event->field_event_venue->value();
    if($venue){
        $venue_path = array_reverse(taxonomy_get_parents_all($venue->tid));
        $first = true;
        foreach($venue_path as $v){
            if(!$first) $out .= ": ";
            else $first = false;
            $out .= $v->name;
            if(isset($v->field_venue_manager['und'])){
                foreach($v->field_venue_manager['und'] as $m){
                    $venue_managers[$m['target_id']] = user_load($m['target_id']);
                }
            }   
        }
    }
    //$out .= $event->field_event_venue->value();
    $out .= "</td>";
    
    // venue approved
    $out .= "<td>";
    if($event->field_venue_manager_approved->value()){
        $out .= 'Approved';
    }else{
        $out .= 'Pending';
    }
    $out .= "</td>";
    
    // venue managers
    $out .= "<td>";
    if(count($venue_managers) > 0){

        $first = true;
        foreach($venue_managers as $manager){
            
            if(!$first) $out .= ', ';
            else $first = false;

            $out .= '<a href="mailto:'. $manager->mail .'" >';
            $out .= $manager->name;
            $out .= '</a>';

        } 
    }else{
        $out .= "~";
    }

    // category
    $category_managers = array();
    $out .= "<td>";
    $category = $event->field_event_category->value();
    if($category){
        $cat_path = array_reverse(taxonomy_get_parents_all($category->tid));
        $first = true;
        foreach($cat_path as $c){
            if(!$first) $out .= ": ";
            else $first = false;
            $out .= $c->name;

            // grab the managers as they go past
            if(isset($c->field_event_category_manager['und'])){
                foreach($c->field_event_category_manager['und'] as $m){
                    $category_managers[$m['target_id']] = user_load($m['target_id']);
                }
            }

        }
    }
    $out .= "</td>";

    // category approved
    $out .= "<td>";
    if($event->field_category_manager_approved->value()){
        $out .= 'Approved';
    }else{
        $out .= 'Pending';
    }
    $out .= "</td>";
    
    // cat managers
    $out .= "<td>";
    if(count($category_managers) > 0){
        $first = true;
        foreach($category_managers as $manager){
            
            if(!$first) $out .= ', ';
            else $first = false;

            $out .= '<a href="mailto:'. $manager->mail .'" >';
            $out .= $manager->name;
            $out .= '</a>';
        } 
    }else{
        $out .= "~";
    }

    $out .= "</td>";
    
    $out .= "<td>";
    $status = $event->field_event_status->value();
    if($status){
        $out .= $status->name;
    }
    $out .= "</td>";
    
    
    $out .= "</tr>";
    
    return $out;
}

function rbge_cal_lists_header(){
    $out = <<<END

<table id="rbge-cal-list">
    <tr>
        <th>id</th>
        <th>Title</th>
        
        <th>Starts</th>
        <th>Repeats</th>
        
        <th>Venue</th>
        <th>VApproval</th>
        <th>VManagers</th>
        
        <th>Category</th>
        <th>CApproval</th>
        <th>CManagers</th>
        
        <th>Status</th>
    
    </tr>
    <tbody>
    
END;

    return $out;

}

function rbge_cal_lists_footer(){
    $out = <<<END
    </tbody>
</table>
    
END;

    return $out;

}



