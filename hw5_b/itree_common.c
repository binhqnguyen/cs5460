/* Generic part */

typedef struct {
	block_t	*p;//pointer to first level block
	block_t	key;//pointer to real block. 0 IF NOT IN BUFFER (init val = 0)
	struct buffer_head *bh;
} Indirect;

typedef struct{
	block_t bg;
	int len;
	int offset; //specify which extent this is in the inode.
} Extent; //An extent in the inode: <block value, len>

static DEFINE_RWLOCK(pointers_lock);

/* Check if a sector is in the range*/
static int is_valid_block(struct inode *inode, long block){
	char b[BDEVNAME_SIZE];
	struct super_block *sb = inode->i_sb;
	int retval = 1;

	if (block < 0) {
		printk("MINIX-fs: block_to_path: block %ld < 0 on dev %s\n",block, bdevname(sb->s_bdev, b));
		retval = 0;
		
	} else 
	if (block > NBLOCKS_INODE_MAX){	
		if (printk_ratelimit())
			printk(KERN_EMERG "MINIX-fs: block_to_path: "
			       "block %ld too big on dev %s\n",
				block, bdevname(sb->s_bdev, b));
		retval = 0;
	}
	return retval;
}

static inline void add_chain(Indirect *p, struct buffer_head *bh, block_t *v)
{
	p->key = *(p->p = v);
	p->bh = bh;
}

static inline int verify_chain(Indirect *from, Indirect *to)
{
	while (from <= to && from->key == *from->p)
		from++;
	return (from > to);
}

static inline block_t *block_end(struct buffer_head *bh)
{
	return (block_t *)((char*)bh->b_data + bh->b_size);
}

static inline Indirect *get_branch(struct inode *inode,
					int depth,
					int *offsets,
					Indirect chain[DEPTH],
					int *err)
{
	struct super_block *sb = inode->i_sb;
	Indirect *p = chain;
	struct buffer_head *bh;

	*err = 0;
	/* i_data is not going away, no lock needed */
	add_chain (chain, NULL, i_data(inode) + *offsets);
	if (!p->key)
		goto no_block;
	while (--depth) {
		bh = sb_bread(sb, block_to_cpu(p->key));
		if (!bh)
			goto failure;
		read_lock(&pointers_lock);
		if (!verify_chain(chain, p))
			goto changed;
		add_chain(++p, bh, (block_t *)bh->b_data + *++offsets);
		read_unlock(&pointers_lock);
		if (!p->key)
			goto no_block;
	}
	return NULL;

changed:
	read_unlock(&pointers_lock);
	brelse(bh);
	*err = -EAGAIN;
	goto no_block;
failure:
	*err = -EIO;
no_block:
	return p;
}
/*
static int alloc_branch(struct inode *inode,
			     int num,
			     int *offsets,
			     Indirect *branch)
{
	int n = 0;
	int i;
	int parent = minix_new_block(inode);

	branch[0].key = cpu_to_block(parent);
	if (parent) for (n = 1; n < num; n++) {
		struct buffer_head *bh;
		// Allocate the next block /
		int nr = minix_new_block(inode);
		if (!nr)
			break;
		branch[n].key = cpu_to_block(nr);
		bh = sb_getblk(inode->i_sb, parent);
		lock_buffer(bh);
		memset(bh->b_data, 0, bh->b_size);
		branch[n].bh = bh;
		branch[n].p = (block_t*) bh->b_data + offsets[n];
		*branch[n].p = branch[n].key;
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		mark_buffer_dirty_inode(bh, inode);
		parent = nr;
	}
	if (n == num)
		return 0;

	// Allocation failed, free what we already allocated //
	for (i = 1; i < n; i++)
		bforget(branch[i].bh);
	for (i = 0; i < n; i++)
		minix_free_block(inode, block_to_cpu(branch[i].key));
	return -ENOSPC;
}
*/
static inline int splice_branch(struct inode *inode,
				     Indirect chain[DEPTH],
				     Indirect *where,
				     int num)
{
	int i;

	write_lock(&pointers_lock);

	/* Verify that place we are splicing to is still there and vacant */
	if (!verify_chain(chain, where-1) || *where->p)
		goto changed;

	*where->p = where->key;

	write_unlock(&pointers_lock);

	/* We are done with atomic stuff, now do the rest of housekeeping */

	inode->i_ctime = CURRENT_TIME_SEC;

	/* had we spliced it onto indirect block? */
	if (where->bh)
		mark_buffer_dirty_inode(where->bh, inode);

	mark_inode_dirty(inode);
	return 0;

changed:
	write_unlock(&pointers_lock);
	for (i = 1; i < num; i++)
		bforget(where[i].bh);
	for (i = 0; i < num; i++)
		minix_free_block(inode, block_to_cpu(where[i].key));
	return -EAGAIN;
}
/*
static void print_inode(struct inode *inode){
	int i  = 0 ;
	printk(KERN_EMERG "inode: ");
	for (i=0;i<DIRECT;++i){
		printk(KERN_EMERG "%d\t", (int)*(i_data(inode)+i));
	}
	printk(KERN_EMERG "\n");
}
*/

/* Get the lastest extent in the inode*/
static Extent get_last_extent(struct inode *inode){
	block_t bl_value = 0;
	block_t bg = 0;
	int len = 0, sec = 0;
	Extent retval;
	retval.bg = 0;
	retval.len = 0;
	retval.offset = -1;

	while ( sec < DIRECT) { //check all non empty sector in inode.
		bl_value = *(i_data(inode)+sec);
		bg = bl_value >> LEN;
		len = (bl_value << BLOCK_BITS ) >> BLOCK_BITS;	
		if (bg == 0){ //hit the first empty extent	
			if (sec==0) goto ret;	//if inode is empty
			bl_value = *(i_data(inode)+sec-1);
			bg = bl_value >> LEN;
			len = (bl_value << BLOCK_BITS ) >> BLOCK_BITS;	
			retval.bg = bg;	
			retval.len = len;
			retval.offset = sec-1;
			goto ret;
		} 
		sec++; //next sector
	}
ret:
	return retval;	

}

/* Mark the inode dirty to update its extents*/
static int update_inode(struct inode *inode, Extent last_extent, int not_contiguous){
	int retval = 0;
	block_t block_val = 0;
	block_val = ((int)last_extent.bg << LEN) | last_extent.len;
	minix_i(inode)->u.i2_data[last_extent.offset+not_contiguous] = (int)block_val;
	mark_inode_dirty(inode);
	return retval;
}

/* Allocating a new extent to store the block_bg
 * Increase the length of the extent if new block is contiguous.
 * Create a new extent if the new block is NOT contiguous.
 */
static int alloc_extent(struct inode *inode, int *block_bg){
	Extent last_extent;
	*block_bg = minix_new_block(inode);
	if (*block_bg!=0){ //succeed allocated the block
		last_extent = get_last_extent(inode);
		if ( *block_bg == (last_extent.bg+last_extent.len+1) ){ //if new block is  contiguous with existing extent, increase extent's length
			last_extent.len++;
			if (last_extent.len >= MAX_LEN){ //this extent is too large (>16), create a new one.
				printk(KERN_EMERG "Extent exceed the MAX_LEN\n");
				goto not_contiguous;
			}
			update_inode(inode,last_extent,0);
			return 0;
		}
not_contiguous: 	//new block is not contiguous, simply create a new extent.
		last_extent.bg = *block_bg;
		last_extent.len = 0;
		update_inode(inode, last_extent,1);
		return 0;
	}
	return -ENOSPC;
}

/*Get a block value from a given sector number*/
/*Return 0 if not found*/
static block_t get_block_value(struct inode *inode, sector_t block){
	uint32_t bl_value = 0;
	uint32_t bg = 0, len = 0, sum_len = 0, sec = 0, retval = 0;
	while ( (sec < DIRECT) ){ //scane all non empty sectors in inode to look for the finding sector.
		bl_value = *(i_data(inode)+sec);
		bg = bl_value >> LEN;
		len = (bl_value << BLOCK_BITS ) >> BLOCK_BITS;
		sum_len += len;
		if (sum_len > block){ //passed the finding sector
			sum_len -= len; 
			retval = (int) (bg + block - sum_len);
			return retval; //bg = begin block of the extent. (block-sum_len) = the position of the finding sector. 
		} 
		sec++; //next sector
	}
	return 0;
}

/*read a block from the inode
 *if not in inode, allocate a new one, and add it to extent
 */
static inline int get_block(struct inode * inode, sector_t block,
			struct buffer_head *bh, int create)
{
	int err = -EIO;
	int block_bg = 0;

	if (!is_valid_block(inode, block))/*check for valid block*/
		goto out;
	block_bg = get_block_value(inode, block);/*get the value contained in the block from the extents on the inode*/
reread:
	if (block_bg!=0){	//block!=0, found on disk
		map_bh(bh, inode->i_sb, block_bg);	
		err = 0; 	//succeed.
		goto out;
	}
	/* Next simple case - plain lookup or failed read of indirect block */
	if (!create) {
		printk(KERN_INFO "itree_common->get_block: can't find sector %d on inode\n",(int)block);
		return err;
	}
	/*Block not found and creating needed, create a new block*/
	/*A block contains both block number and length of region*/
	err = alloc_extent(inode, &block_bg);	
	if (err==0)	//succeed, reread block again
		goto reread;
	else{	//failed allocating a new block
		printk(KERN_INFO "itree_common->get_block: failed allocating new block\n");
		minix_free_block(inode,block);
		err = -ENOSPC;
		goto out;
	}
out:
	return err;
}

static inline int all_zeroes(block_t *p, block_t *q)
{
	while (p < q)
		if (*p++)
			return 0;
	return 1;
}

/*
static Indirect *find_shared(struct inode *inode,
				int depth,
				int offsets[DEPTH],
				Indirect chain[DEPTH],
				block_t *top)
{
	Indirect *partial, *p;
	int k, err;

	*top = 0;
	for (k = depth; k > 1 && !offsets[k-1]; k--)
		;
	partial = get_branch(inode, k, offsets, chain, &err);

	write_lock(&pointers_lock);
	if (!partial)
		partial = chain + k-1;
	if (!partial->key && *partial->p) {
		write_unlock(&pointers_lock);
		goto no_top;
	}
	for (p=partial;p>chain && all_zeroes((block_t*)p->bh->b_data,p->p);p--)
		;
	if (p == chain + k - 1 && p > chain) {
		p->p--;
	} else {
		*top = *p->p;
		*p->p = 0;
	}
	write_unlock(&pointers_lock);

	while(partial > p)
	{
		brelse(partial->bh);
		partial--;
	}
no_top:
	return partial;
}
*/
static inline void free_data(struct inode *inode, block_t *p, block_t *q)
{
	unsigned long nr;

	for ( ; p < q ; p++) {
		nr = block_to_cpu(*p);
		if (nr) {
			*p = 0;
			minix_free_block(inode, nr);
		}
	}
}
/*
static void free_branches(struct inode *inode, block_t *p, block_t *q, int depth)
{
	struct buffer_head * bh;
	unsigned long nr;

	if (depth--) {
		for ( ; p < q ; p++) {
			nr = block_to_cpu(*p);
			if (!nr)
				continue;
			*p = 0;
			bh = sb_bread(inode->i_sb, nr);
			if (!bh)
				continue;
			free_branches(inode, (block_t*)bh->b_data,
				      block_end(bh), depth);
			bforget(bh);
			minix_free_block(inode, nr);
			mark_inode_dirty(inode);
		}
	} else
		free_data(inode, p, q);
}
*/
static inline void truncate (struct inode * inode)
{
	block_t *idata = i_data(inode);
	block_t bl_value = 0;
	block_t bg = 0;
	int len = 0;
	int i = 0;
	int offset = 0;	

	//scane through all the extents in the inode
	while (offset < DIRECT) {
		bl_value = idata[offset];
		bg = bl_value >> LEN;
		len = (bl_value << BLOCK_BITS) >> BLOCK_BITS;
		if (bl_value == 0 || bg == 0){ //if already empty skip
			offset++;
			continue; 
		}
		idata[offset] = 0; //mark inode zones free (0)
		for (i=-1; i <= MAX_LEN; ++i){ //free all the blocks recorded in the extent.
			minix_free_block(inode, bg);
			bg++;
		}
		offset++;
	}		
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(inode);
	return;
}

static inline unsigned nblocks(loff_t size, struct super_block *sb)
{
	int k = sb->s_blocksize_bits - 10;	//s_blocksize_bits=10.
	unsigned blocks, res;
	blocks = (size + sb->s_blocksize - 1) >> (BLOCK_SIZE_BITS + k);	//k=-4, BLOCK_SIZE_BITS=10 (1024B block).
	res = blocks;
	return res;
}

