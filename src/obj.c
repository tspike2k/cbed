//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "obj.h"
#include "files.h"

// TODO: Though this does work, it would be less error prone if we stopped referring to
// attributes using their cryptic OBJ short-hand (v, t, vn).

typedef struct {
    s32 v_index;
    s32 vt_index;
    s32 vn_index;
} Obj__Face_Vertex;

typedef struct {
    Obj__Face_Vertex vertex[4];
} Obj__Face;

// There are at least three different ways to define a "v" entry in an OBJ file:
// - 3 floats: x, y, z vertex positions
// - 4 floats: x, y, z, w vertex positions
// - 6 floats: x, y, z vertex positions and r, g, b color values.
// If unspecified, w defaults to 1.
typedef struct{
    f32 data[6];
    u32 count;
} Obj__V;

typedef struct{
    f32 data[3];
} Obj__3F;

typedef struct{
    Obj__Face *faces;

    size_t   v_count;
    Obj__V  *v;
    size_t   vt_count;
    Obj__3F *vt;
    size_t   vn_count;
    Obj__3F *vn;
} Obj__Internal;

static String obj__eat_between_whitespace(String *reader){
    while(reader->size && char_is_whitespace(reader->text[0])){
        reader->text++;
        reader->size--;
    }

    String result = *reader;
    while(reader->size && !char_is_whitespace(reader->text[0])){
        reader->text++;
        reader->size--;
    }
    result.size = reader->text - result.text;

    return result;
}

static u32 obj__parse_f32_line(String line, f32 *data, u32 data_max){
    // TODO: Return the total number of items on a line, even if we don't have room to store them
    // all. This would allow us to report if a vertex attribute is malformed.
    u32 count = 0;
    while(line.size && count < data_max){
        String s = obj__eat_between_whitespace(&line);
        if(!str_to_f32(s.text, s.size, &data[count++])){
            // TODO: Should we do better error reporting here? If we returned a bool, this
            // would allow us to log that a parsing error occured rather than being ambilgous
            // as to wheather it's a parsing error or too few components specified for the
            // vertex attribute.
            count = 0; // This will trigger an error if we can't correctly parse as an f32.
            break;
        }
    }
    return count;
}

static void obj__scan_and_allocate(Obj_Data *obj, String reader, Buffer* dest){
    assert(sizeof(obj->internal) >= sizeof(Obj__Internal));
    Obj__Internal *internal = (Obj__Internal *)&obj->internal[0];

    while(reader.size){
        String line = str_eat_line(&reader);
        String cmd = obj__eat_between_whitespace(&line);
        if(str_match(cmd, str_lit("v"))){
            internal->v_count++;
        }
        else if(str_match(cmd, str_lit("vn"))){
            internal->vn_count++;
        }
        else if(str_match(cmd, str_lit("vt"))){
            internal->vt_count++;
        }
        else if(str_match(cmd, str_lit("f"))){
            obj->faces_count++;
        }
    }

    internal->v  = buffer_push_array(Obj__V, dest, internal->v_count);
    internal->vt = buffer_push_array(Obj__3F, dest, internal->vt_count);
    internal->vn = buffer_push_array(Obj__3F, dest, internal->vn_count);
    internal->faces = buffer_push_array(Obj__Face, dest, obj->faces_count);
}

static String obj__eat_until_slash(String *reader){
    String result = *reader;
    while(reader->size){
        if(reader->text[0] == '/'){
            result.size = reader->text - result.text;
            reader->text++;
            reader->size--;
            break;
        }

        reader->text++;
        reader->size--;
    }

    return result;
}

static bool obj__parse_face_vertex_index(String str, s32 *index){
    bool error = false;
    s64 value;
    if(str.size){
        if(str_to_int(str.text, str.size, &value)){
            // TODO: Report if size is too big? Use an s64 for face indeces instead?
            *index = (s32)value;
        }
        else{
            // TODO: Report error.
            error = true;
        }
    }

    return !error;
}

static bool obj__parse_face_vertex(Obj__Face_Vertex *v, String *reader){
    bool error = false;

    String cmd = obj__eat_between_whitespace(reader);
    String str0 = obj__eat_until_slash(&cmd);
    String str1 = obj__eat_until_slash(&cmd);
    String str2 = obj__eat_until_slash(&cmd);

    if(!obj__parse_face_vertex_index(str0, &v->v_index))
        error = true;

    if(!obj__parse_face_vertex_index(str1, &v->vt_index))
        error = true;

    if(!obj__parse_face_vertex_index(str2, &v->vn_index))
        error = true;

    return !error;
}

Ceabed_API bool obj_parse_file(Obj_Data *obj, const char *file_name, Buffer *dest, Buffer* scratch){
    size_t scratch_frame = buffer_frame_begin(scratch);

    assert(sizeof(obj->internal) >= sizeof(Obj__Internal));
    Obj__Internal *internal = (Obj__Internal *)&obj->internal[0];

    String reader = file_read_into_memory(file_name, scratch);
    bool error = reader.size == 0;
    obj__scan_and_allocate(obj, reader, dest);

    size_t v_index  = 0;
    size_t vt_index = 0;
    size_t vn_index = 0;
    size_t f_index  = 0;

    u32 line_count = 0;
    while(reader.size && !error){
        String line = str_eat_line(&reader);
        line_count++;
        String cmd = obj__eat_between_whitespace(&line);

        if(str_match(cmd, str("v"))){
            assert(v_index < internal->v_count);
            Obj__V *v = &internal->v[v_index++];
            v->count = obj__parse_f32_line(line, v->data, Array_Len(v->data));

            if(v->count != 3 && v->count != 4 && v->count != 6){
                // TODO: Error handling (report and propogate)
                error = true;
            }
        }
        else if(str_match(cmd, str("vn"))){
            assert(vn_index < internal->vn_count);
            Obj__3F *vn = &internal->vn[vn_index++];
            size_t count = obj__parse_f32_line(line, vn->data, Array_Len(vn->data));
            if(count != 3){
                error = true;
                // TODO: Error handling (report and propogate)
            }
        }
        else if(str_match(cmd, str("vt"))){
            assert(vt_index < internal->vt_count);
            Obj__3F *vt = &internal->vt[vt_index++];
            size_t count = obj__parse_f32_line(line, vt->data, Array_Len(vt->data));

            // NOTE: If the v and w components default to 0 if not specified.
            // We don't have to set manually as the buffer allocation cleared the memory
            // to zero for us already.
            if(count != 1 && count != 2 && count != 3){
                error = true;
            }
        }
        else if(str_match(cmd, str("f"))){
            assert(f_index < obj->faces_count);
            Obj__Face *f = &internal->faces[f_index++];
            // TODO: Report and error out if there are more vertices defined for a face
            // than we can handle? This seems to be allowed in the OBJ "spec."
            u32 vertex_count = 0;
            while(line.size && vertex_count < Array_Len(f->vertex)){
                Obj__Face_Vertex *fv = &f->vertex[vertex_count++];
                if(!obj__parse_face_vertex(fv, &line)){
                    // TODO: Report error? We probably will from within obj__parse_face_vertex...
                    error = true;
                }
            }

            if(vertex_count != 3 && vertex_count != 4){
                // TODO: Report error
                error = true;
            }

            // TODO: Check to make sure vertex attributes are consistent. For instance, each
            // face should have a valid position index, and the other attributes should be
            // consistently ommited or not.

            // TODO: Pre-check indeces to make sure they're within a valid range
        }
    }

    buffer_frame_end(scratch, scratch_frame);
    return !error;
}

static size_t obj__resolve_index(s32 index, size_t count){
    assert(index != 0);
    size_t result = 0;

    // TODO: Negative indeces means we are using relatives offset from the end of the array.
    // Implement this.
    assert(index > 0);

    result = index - 1;

    // NOTE: This should never happen once we pre-checked the indeces during the parsing phase.
    assert(result < count);
    return result;
}

Ceabed_API Obj_Polygon obj_polygon_from_face(Obj_Data *obj, size_t face_index){
    assert(sizeof(obj->internal) >= sizeof(Obj__Internal));
    Obj__Internal *internal = (Obj__Internal *)&obj->internal[0];

    assert(face_index < obj->faces_count);
    Obj__Face *face = &internal->faces[face_index];

    Obj_Polygon result = {};
    result.vertex_count = 3;

    // If the index for the fourth vertex position is non-zero, this means we have quad.
    if(face->vertex[3].v_index != 0){
        result.vertex_count = 4;
    }

    for_u32(i, result.vertex_count){
        Obj_Vertex *dest    = &result.vertex[i];
        Obj__Face_Vertex *src = &face->vertex[i];

        size_t v_index  = obj__resolve_index(src->v_index, internal->v_count);
        Obj__V *v = &internal->v[v_index];

        dest->position = v4(v->data[0], v->data[1], v->data[2], 1);
        if(v->count == 4){
            dest->position.w = v->data[3];
        }
        else if(v->count == 6){
            result.flags |= Obj_Flag_Color;

            dest->color = v3(v->data[3], v->data[4], v->data[5]);
        }
        else if(v->count != 3){
            assert(0);
        }

        if(src->vt_index != 0){
            result.flags |= Obj_Flag_Texture;

            size_t vt_index = obj__resolve_index(src->vt_index, internal->vt_count);
            Obj__3F vt = internal->vt[vt_index];
            dest->texture = v3(vt.data[0], vt.data[1], vt.data[2]);
        }

        if(src->vn_index != 0){
            result.flags |= Obj_Flag_Normal;

            size_t vn_index = obj__resolve_index(src->vn_index, internal->vn_count);
            Obj__3F vn = internal->vn[vn_index];
            dest->normal = v3(vn.data[0], vn.data[1], vn.data[2]);
        }
    }

    return result;
}
